#include "ui/menu_system.h"
#include "ui/text_renderer.h"
#include "ui/config_manager.h"

MenuSystem::MenuSystem(SDL_Renderer* renderer, TextRenderer* text_renderer)
    : renderer_(renderer), text_renderer_(text_renderer) {
    build_main_menu();
    current_menu_ = &main_menu_;
}

MenuSystem::~MenuSystem() = default;

void MenuSystem::show() { visible_ = true; selected_index_ = 0; }
void MenuSystem::hide() { 
    visible_ = false; 
    while (!menu_stack_.empty()) menu_stack_.pop(); 
    current_menu_ = &main_menu_; 
    selected_index_ = 0;
}

crayon::MenuAction MenuSystem::process_input(SDL_Keycode key) {
    if (!visible_) return crayon::MenuAction::None;
    
    switch (key) {
        case SDLK_UP: navigate_up(); break;
        case SDLK_DOWN: navigate_down(); break;
        case SDLK_RETURN: 
        case SDLK_SPACE: {
            auto action = select_current();
            if (action != crayon::MenuAction::None) {
                hide(); // Close menu after action
                return action;
            }
            break;
        }
        case SDLK_ESCAPE: 
        case SDLK_BACKSPACE:
            go_back(); 
            break;
    }
    return crayon::MenuAction::None;
}

int MenuSystem::get_selected_slot() const { return last_selected_slot_; }

void MenuSystem::render() {
    if (!visible_ || !current_menu_ || !text_renderer_) return;
    
    // Get window size
    int window_width, window_height;
    SDL_GetRendererOutputSize(renderer_, &window_width, &window_height);
    
    // Menu dimensions - compact like Videopac
    const int menu_width = 300;
    const int item_height = 17;
    const int menu_height = static_cast<int>(current_menu_->size()) * item_height + 40;
    const int menu_x = (window_width - menu_width) / 2;
    const int menu_y = (window_height - menu_height) / 2;
    
    // Draw semi-transparent background overlay
    SDL_SetRenderDrawBlendMode(renderer_, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(renderer_, 0, 0, 0, 200);
    SDL_Rect overlay = {0, 0, window_width, window_height};
    SDL_RenderFillRect(renderer_, &overlay);
    
    // Draw menu background
    SDL_SetRenderDrawColor(renderer_, 30, 30, 40, 255);
    SDL_Rect menu_bg = {menu_x, menu_y, menu_width, menu_height};
    SDL_RenderFillRect(renderer_, &menu_bg);
    
    // Draw menu border
    SDL_SetRenderDrawColor(renderer_, 80, 80, 100, 255);
    SDL_RenderDrawRect(renderer_, &menu_bg);
    
    // Draw title
    SDL_Color title_color = {220, 220, 255, 255};
    text_renderer_->render_text("Crayon Menu (F1)", menu_x + menu_width / 2, menu_y + 6, 
                                title_color, TextRenderer::FontSize::Small, 
                                TextRenderer::TextAlign::Center);
    
    // Draw menu items
    SDL_Color normal_color = {180, 180, 180, 255};
    SDL_Color selected_color = {255, 255, 100, 255};
    SDL_Color disabled_color = {80, 80, 80, 255};
    
    for (size_t i = 0; i < current_menu_->size(); ++i) {
        const auto& item = (*current_menu_)[i];
        int item_y = menu_y + 24 + static_cast<int>(i) * item_height;
        
        // Draw selection highlight
        if (static_cast<int>(i) == selected_index_) {
            SDL_SetRenderDrawColor(renderer_, 60, 60, 80, 255);
            SDL_Rect highlight = {menu_x + 3, item_y - 1, menu_width - 6, item_height - 2};
            SDL_RenderFillRect(renderer_, &highlight);
        }
        
        // Draw menu item text
        SDL_Color color = item.enabled ? 
            (static_cast<int>(i) == selected_index_ ? selected_color : normal_color) : 
            disabled_color;
        
        std::string display_text = item.label;
        if (!item.value.empty()) {
            display_text += ": " + item.value;
        }
        if (item.has_submenu) {
            display_text += " >";
        }
        
        text_renderer_->render_text(display_text, menu_x + 15, item_y + 2, color, 
                                    TextRenderer::FontSize::Small, 
                                    TextRenderer::TextAlign::Left);
    }
    
    // Draw instructions at bottom
    SDL_Color instruction_color = {120, 120, 140, 255};
    text_renderer_->render_text("Up/Down: Navigate  Enter: Select  Esc: Back", 
                                menu_x + menu_width / 2, menu_y + menu_height - 13, 
                                instruction_color, TextRenderer::FontSize::Small, 
                                TextRenderer::TextAlign::Center);
}

void MenuSystem::build_main_menu() {
    main_menu_.clear();
    
    // File menu with hotkeys
    main_menu_.emplace_back("Load BASIC ROM (F2)", crayon::MenuAction::LoadBasicROM);
    main_menu_.emplace_back("Load Monitor ROM (F3)", crayon::MenuAction::LoadMonitorROM);
    main_menu_.emplace_back("Load Cartridge (F4)", crayon::MenuAction::LoadCartridge);
    main_menu_.emplace_back("Load K7 Cassette (F6)", crayon::MenuAction::LoadK7);
    
    // Emulation control
    main_menu_.emplace_back("Reset (F7)", crayon::MenuAction::Reset);
    main_menu_.emplace_back("Pause/Resume (F8)", crayon::MenuAction::Pause);
    
    // Save states submenu
    MenuItem save_state_menu("Save State (F9)", crayon::MenuAction::None);
    save_state_menu.has_submenu = true;
    for (int i = 0; i < 10; ++i) {
        MenuItem slot("Slot " + std::to_string(i) + " (" + std::to_string(i) + ")", crayon::MenuAction::SaveState);
        slot.slot_number = i;
        save_state_menu.submenu.push_back(slot);
    }
    main_menu_.push_back(save_state_menu);
    
    // Load states submenu
    MenuItem load_state_menu("Load State (F10)", crayon::MenuAction::None);
    load_state_menu.has_submenu = true;
    for (int i = 0; i < 10; ++i) {
        MenuItem slot("Slot " + std::to_string(i) + " (" + std::to_string(i) + ")", crayon::MenuAction::LoadState);
        slot.slot_number = i;
        load_state_menu.submenu.push_back(slot);
    }
    main_menu_.push_back(load_state_menu);
    
    // Display options
    main_menu_.emplace_back("Screenshot (F11)", crayon::MenuAction::Screenshot);
    main_menu_.emplace_back("Toggle FPS (F12)", crayon::MenuAction::ToggleFPS);
    main_menu_.emplace_back("Toggle Fullscreen (Alt+Enter)", crayon::MenuAction::ToggleFullscreen);
    main_menu_.emplace_back("Toggle Debugger (F5)", crayon::MenuAction::ToggleDebugger);
    
    // Exit
    main_menu_.emplace_back("Quit (Esc)", crayon::MenuAction::Quit);
}

void MenuSystem::update_menu_values(ConfigManager* config) {
    if (!config) return;
    
    // Update menu items with current config values
    for (auto& item : main_menu_) {
        if (item.label == "Toggle FPS") {
            item.value = config->get_fps_display_enabled() ? "ON" : "OFF";
        } else if (item.label == "Toggle Fullscreen") {
            item.value = config->get_fullscreen() ? "ON" : "OFF";
        }
    }
}

void MenuSystem::navigate_up() { 
    if (selected_index_ > 0) {
        selected_index_--;
    } else if (current_menu_ && !current_menu_->empty()) {
        // Wrap to bottom
        selected_index_ = static_cast<int>(current_menu_->size()) - 1;
    }
}

void MenuSystem::navigate_down() {
    if (current_menu_ && selected_index_ < static_cast<int>(current_menu_->size()) - 1) {
        selected_index_++;
    } else {
        // Wrap to top
        selected_index_ = 0;
    }
}

crayon::MenuAction MenuSystem::select_current() {
    if (!current_menu_ || current_menu_->empty()) return crayon::MenuAction::None;
    if (selected_index_ < 0 || selected_index_ >= static_cast<int>(current_menu_->size())) 
        return crayon::MenuAction::None;
    
    auto& item = (*current_menu_)[selected_index_];
    
    if (!item.enabled) return crayon::MenuAction::None;
    
    if (item.has_submenu && !item.submenu.empty()) {
        // Navigate into submenu
        menu_stack_.push(current_menu_);
        current_menu_ = &item.submenu;
        selected_index_ = 0;
        return crayon::MenuAction::None;
    } else {
        // Execute action
        last_selected_slot_ = item.slot_number;
        last_action_ = item.action;
        return item.action;
    }
}

void MenuSystem::go_back() {
    if (!menu_stack_.empty()) {
        current_menu_ = menu_stack_.top();
        menu_stack_.pop();
        selected_index_ = 0;
    } else {
        hide();
    }
}
