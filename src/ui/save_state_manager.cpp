#include "ui/save_state_manager.h"
#include "ui/text_renderer.h"
#include "emulator_core.h"
#include "savestate.h"
#include <filesystem>
#include <ctime>
#include <sstream>
#include <iomanip>

SaveStateManagerUI::SaveStateManagerUI(crayon::EmulatorCore* emulator, SDL_Renderer* renderer, TextRenderer* text_renderer)
    : emulator_(emulator), renderer_(renderer), text_renderer_(text_renderer) {
    ensure_saves_directory();
}

SaveStateManagerUI::~SaveStateManagerUI() {
    free_thumbnail_textures();
}

crayon::Result<void> SaveStateManagerUI::save_state(int slot, const std::string& cartridge_name) {
    auto filename = get_state_filename(cartridge_name, slot);
    auto result = emulator_->save_state(filename);
    
    if (result.is_ok()) {
        // Capture thumbnail after successful save
        auto thumb_filename = get_thumbnail_filename(cartridge_name, slot);
        capture_thumbnail(thumb_filename);
    }
    
    return result;
}

crayon::Result<void> SaveStateManagerUI::load_state(int slot, const std::string& cartridge_name) {
    auto filename = get_state_filename(cartridge_name, slot);
    return emulator_->load_state(filename);
}

crayon::Result<void> SaveStateManagerUI::delete_state(int slot, const std::string& cartridge_name) {
    auto filename = get_state_filename(cartridge_name, slot);
    if (std::filesystem::exists(filename)) {
        std::filesystem::remove(filename);
        auto thumb = get_thumbnail_filename(cartridge_name, slot);
        if (std::filesystem::exists(thumb)) {
            std::filesystem::remove(thumb);
        }
    }
    return crayon::Result<void>::ok();
}

std::vector<SaveStateInfo> SaveStateManagerUI::list_states(const std::string& cartridge_name) {
    std::vector<SaveStateInfo> states;
    for (int i = 0; i < 10; ++i) {
        SaveStateInfo info;
        info.slot = i;
        info.filename = get_state_filename(cartridge_name, i);
        info.thumbnail_path = get_thumbnail_filename(cartridge_name, i);
        info.exists = std::filesystem::exists(info.filename);
        info.thumbnail_texture = nullptr;
        
        if (info.exists) {
            auto ftime = std::filesystem::last_write_time(info.filename);
            auto sctp = std::chrono::time_point_cast<std::chrono::system_clock::duration>(
                ftime - std::filesystem::file_time_type::clock::now() + std::chrono::system_clock::now());
            info.timestamp = std::chrono::system_clock::to_time_t(sctp);
        } else {
            info.timestamp = 0;
        }
        states.push_back(info);
    }
    return states;
}

std::string SaveStateManagerUI::get_saves_directory() const { 
    return saves_dir_; 
}

std::string SaveStateManagerUI::get_state_filename(const std::string& cartridge_name, int slot) {
    return saves_dir_ + "/" + cartridge_name + "_slot" + std::to_string(slot) + ".sav";
}

std::string SaveStateManagerUI::get_thumbnail_filename(const std::string& cartridge_name, int slot) {
    return saves_dir_ + "/" + cartridge_name + "_slot" + std::to_string(slot) + ".bmp";
}

crayon::Result<void> SaveStateManagerUI::capture_thumbnail(const std::string& filename) {
    // Get the current framebuffer from renderer
    int width, height;
    SDL_GetRendererOutputSize(renderer_, &width, &height);
    
    // Create a surface to capture the framebuffer
    SDL_Surface* surface = SDL_CreateRGBSurface(0, width, height, 32,
                                                 0x00FF0000, 0x0000FF00, 0x000000FF, 0xFF000000);
    if (!surface) {
        return crayon::Result<void>::err("Failed to create surface for thumbnail");
    }
    
    // Read pixels from renderer
    if (SDL_RenderReadPixels(renderer_, nullptr, SDL_PIXELFORMAT_ARGB8888, 
                             surface->pixels, surface->pitch) != 0) {
        SDL_FreeSurface(surface);
        return crayon::Result<void>::err("Failed to read pixels from renderer");
    }
    
    // Scale down to 64x40 thumbnail
    SDL_Surface* thumbnail = SDL_CreateRGBSurface(0, 64, 40, 32,
                                                   0x00FF0000, 0x0000FF00, 0x000000FF, 0xFF000000);
    if (!thumbnail) {
        SDL_FreeSurface(surface);
        return crayon::Result<void>::err("Failed to create thumbnail surface");
    }
    
    // Simple nearest-neighbor scaling
    SDL_BlitScaled(surface, nullptr, thumbnail, nullptr);
    
    // Save as BMP (SDL built-in, no need for SDL_image)
    std::string bmp_filename = filename;
    size_t ext_pos = bmp_filename.find_last_of('.');
    if (ext_pos != std::string::npos) {
        bmp_filename = bmp_filename.substr(0, ext_pos) + ".bmp";
    } else {
        bmp_filename += ".bmp";
    }
    
    if (SDL_SaveBMP(thumbnail, bmp_filename.c_str()) != 0) {
        SDL_FreeSurface(surface);
        SDL_FreeSurface(thumbnail);
        return crayon::Result<void>::err("Failed to save thumbnail BMP");
    }
    
    SDL_FreeSurface(surface);
    SDL_FreeSurface(thumbnail);
    return crayon::Result<void>::ok();
}

bool SaveStateManagerUI::ensure_saves_directory() {
    saves_dir_ = "userdata/saves";
    if (!std::filesystem::exists(saves_dir_)) {
        std::filesystem::create_directories(saves_dir_);
    }
    return std::filesystem::exists(saves_dir_);
}

void SaveStateManagerUI::load_thumbnail_textures(const std::vector<SaveStateInfo>& states) {
    free_thumbnail_textures();
    cached_states_ = states;
    
    for (auto& state : cached_states_) {
        if (state.exists && std::filesystem::exists(state.thumbnail_path)) {
            SDL_Surface* surface = SDL_LoadBMP(state.thumbnail_path.c_str());
            if (surface) {
                state.thumbnail_texture = SDL_CreateTextureFromSurface(renderer_, surface);
                SDL_FreeSurface(surface);
            }
        }
    }
}

void SaveStateManagerUI::free_thumbnail_textures() {
    for (auto& state : cached_states_) {
        if (state.thumbnail_texture) {
            SDL_DestroyTexture(state.thumbnail_texture);
            state.thumbnail_texture = nullptr;
        }
    }
    cached_states_.clear();
}

std::string SaveStateManagerUI::format_timestamp(std::time_t timestamp) const {
    if (timestamp == 0) return "Empty Slot";
    
    std::tm* tm_info = std::localtime(&timestamp);
    std::ostringstream oss;
    oss << std::put_time(tm_info, "%Y-%m-%d %H:%M:%S");
    return oss.str();
}

void SaveStateManagerUI::render_ui(const std::string& cartridge_name) {
    if (!show_ui_) return;
    
    // Load states if not cached
    if (cached_states_.empty()) {
        auto states = list_states(cartridge_name);
        load_thumbnail_textures(states);
    }
    
    // Semi-transparent overlay
    SDL_SetRenderDrawBlendMode(renderer_, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(renderer_, 0, 0, 0, 180);
    int window_width, window_height;
    SDL_GetRendererOutputSize(renderer_, &window_width, &window_height);
    SDL_Rect overlay = {0, 0, window_width, window_height};
    SDL_RenderFillRect(renderer_, &overlay);
    
    // Panel
    SDL_SetRenderDrawColor(renderer_, 40, 40, 40, 255);
    SDL_Rect panel = {50, 30, window_width - 100, window_height - 60};
    SDL_RenderFillRect(renderer_, &panel);
    SDL_SetRenderDrawColor(renderer_, 100, 100, 100, 255);
    SDL_RenderDrawRect(renderer_, &panel);
    
    // Title
    SDL_Color white = {255, 255, 255, 255};
    std::string title = is_save_mode_ ? "Save State" : "Load State";
    text_renderer_->render_text(renderer_, title.c_str(), 
                                window_width / 2, 50, white, TextRenderer::TextAlign::Center);
    
    // Instructions
    SDL_Color gray = {180, 180, 180, 255};
    text_renderer_->render_text(renderer_, "UP/DOWN: Select Slot | ENTER: Confirm | DEL: Delete | ESC: Cancel", 
                                window_width / 2, 80, gray, TextRenderer::TextAlign::Center);
    
    // Render save slots in a grid (2 columns, 5 rows)
    int slot_width = 350;
    int slot_height = 90;
    int start_x = 100;
    int start_y = 120;
    int spacing = 20;
    
    for (int i = 0; i < 10; ++i) {
        int col = i % 2;
        int row = i / 2;
        int x = start_x + col * (slot_width + spacing);
        int y = start_y + row * (slot_height + spacing);
        
        const auto& state = cached_states_[i];
        
        // Highlight selected slot
        if (i == selected_slot_) {
            SDL_SetRenderDrawColor(renderer_, 60, 60, 120, 255);
            SDL_Rect highlight = {x - 5, y - 5, slot_width + 10, slot_height + 10};
            SDL_RenderFillRect(renderer_, &highlight);
        }
        
        // Slot box
        SDL_SetRenderDrawColor(renderer_, 50, 50, 50, 255);
        SDL_Rect slot_box = {x, y, slot_width, slot_height};
        SDL_RenderFillRect(renderer_, &slot_box);
        SDL_SetRenderDrawColor(renderer_, 120, 120, 120, 255);
        SDL_RenderDrawRect(renderer_, &slot_box);
        
        // Thumbnail
        SDL_Rect thumb_rect = {x + 5, y + 5, 64, 40};
        if (state.thumbnail_texture) {
            SDL_RenderCopy(renderer_, state.thumbnail_texture, nullptr, &thumb_rect);
        } else {
            SDL_SetRenderDrawColor(renderer_, 30, 30, 30, 255);
            SDL_RenderFillRect(renderer_, &thumb_rect);
        }
        
        // Slot info
        std::string slot_text = "Slot " + std::to_string(i);
        text_renderer_->render_text(renderer_, slot_text.c_str(), 
                                    x + 80, y + 10, white, TextRenderer::TextAlign::Left);
        
        std::string time_text = format_timestamp(state.timestamp);
        SDL_Color time_color = state.exists ? SDL_Color{200, 200, 200, 255} : SDL_Color{120, 120, 120, 255};
        text_renderer_->render_text(renderer_, time_text.c_str(), 
                                    x + 80, y + 30, time_color, TextRenderer::TextAlign::Left);
    }
}

bool SaveStateManagerUI::process_input(SDL_Keycode key) {
    if (!show_ui_) return false;
    
    switch (key) {
        case SDLK_UP:
            if (selected_slot_ >= 2) selected_slot_ -= 2;
            return true;
        case SDLK_DOWN:
            if (selected_slot_ < 8) selected_slot_ += 2;
            return true;
        case SDLK_LEFT:
            if (selected_slot_ % 2 == 1) selected_slot_--;
            return true;
        case SDLK_RIGHT:
            if (selected_slot_ % 2 == 0 && selected_slot_ < 9) selected_slot_++;
            return true;
        case SDLK_RETURN:
            // Perform save or load
            // This will be handled by the caller
            show_ui_ = false;
            return true;
        case SDLK_DELETE:
            // Delete the selected slot
            // This will be handled by the caller
            return true;
        case SDLK_ESCAPE:
            show_ui_ = false;
            free_thumbnail_textures();
            return true;
    }
    
    return false;
}
