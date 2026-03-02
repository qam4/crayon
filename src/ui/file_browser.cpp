#include "ui/file_browser.h"
#include "ui/text_renderer.h"
#include "ui/config_manager.h"
#include <filesystem>
#include <algorithm>

namespace fs = std::filesystem;

FileBrowser::FileBrowser(SDL_Renderer* renderer, TextRenderer* text_renderer, ConfigManager* config)
    : renderer_(renderer), text_renderer_(text_renderer), config_(config) {}
FileBrowser::~FileBrowser() = default;

void FileBrowser::open(const std::string& extensions, FileType file_type) {
    extensions_filter_ = extensions;
    current_file_type_ = file_type;
    is_open_ = true;
    file_selected_ = false;
    selected_file_.clear();
    selected_index_ = 0;
    scroll_offset_ = 0;
    
    // Get last directory from config based on file type
    switch (file_type) {
        case FileType::Cartridge:
            current_directory_ = config_->get_last_cartridge_directory();
            break;
        case FileType::ROM:
            current_directory_ = config_->get_last_rom_directory();
            break;
        case FileType::Generic:
            current_directory_ = config_->get_last_k7_directory();
            break;
    }
    
    // Fallback to current directory if empty or doesn't exist
    if (current_directory_.empty() || !fs::exists(current_directory_)) {
        current_directory_ = fs::current_path().string();
    }
    
    scan_directory();
}

void FileBrowser::close() { is_open_ = false; }

bool FileBrowser::process_input(SDL_Keycode key) {
    if (!is_open_) return false;
    switch (key) {
        case SDLK_UP: 
            if (selected_index_ > 0) {
                selected_index_--;
                // Adjust scroll offset if selection moves above visible area
                if (selected_index_ < scroll_offset_) {
                    scroll_offset_ = selected_index_;
                }
            }
            return true;
        case SDLK_DOWN: 
            if (selected_index_ < static_cast<int>(entries_.size()) - 1) {
                selected_index_++;
                // Adjust scroll offset if selection moves below visible area
                int visible_items = 18;
                if (selected_index_ >= scroll_offset_ + visible_items) {
                    scroll_offset_ = selected_index_ - visible_items + 1;
                }
            }
            return true;
        case SDLK_RETURN: select_current_file(); return true;
        case SDLK_BACKSPACE: navigate_up(); return true;
        case SDLK_ESCAPE: close(); return true;
    }
    return false;
}

void FileBrowser::render() {
    if (!is_open_) return;
    
    // Semi-transparent background overlay
    SDL_SetRenderDrawBlendMode(renderer_, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(renderer_, 0, 0, 0, 180);
    SDL_Rect overlay = {0, 0, 960, 600}; // Assuming 960x600 window (3x scale)
    SDL_RenderFillRect(renderer_, &overlay);
    
    // File browser panel
    SDL_SetRenderDrawColor(renderer_, 40, 40, 40, 255);
    SDL_Rect panel = {80, 50, 800, 500};
    SDL_RenderFillRect(renderer_, &panel);
    
    // Border
    SDL_SetRenderDrawColor(renderer_, 100, 100, 100, 255);
    SDL_RenderDrawRect(renderer_, &panel);
    
    // Title
    text_renderer_->render_text(renderer_, "Select File", 480, 70, {255, 255, 255, 255}, TextRenderer::TextAlign::Center);
    
    // Current directory
    text_renderer_->render_text(renderer_, current_directory_.c_str(), 100, 100, {200, 200, 200, 255}, TextRenderer::TextAlign::Left);
    
    // File list
    int y = 130;
    int visible_items = 18;
    int start_index = scroll_offset_;
    int end_index = std::min(start_index + visible_items, static_cast<int>(entries_.size()));
    
    for (int i = start_index; i < end_index; ++i) {
        const auto& entry = entries_[i];
        SDL_Color color = {200, 200, 200, 255};
        
        // Highlight selected item
        if (i == selected_index_) {
            SDL_SetRenderDrawColor(renderer_, 60, 60, 120, 255);
            SDL_Rect highlight = {90, y - 2, 780, 22};
            SDL_RenderFillRect(renderer_, &highlight);
            color = {255, 255, 255, 255};
        }
        
        // Directory indicator or file name
        std::string display_name = entry.is_directory ? "[" + entry.name + "]" : entry.name;
        text_renderer_->render_text(renderer_, display_name.c_str(), 100, y, color, TextRenderer::TextAlign::Left);
        
        y += 24;
    }
    
    // Instructions
    text_renderer_->render_text(renderer_, "UP/DOWN: Navigate | ENTER: Select | BACKSPACE: Parent Dir | ESC: Cancel", 
                                480, 530, {150, 150, 150, 255}, TextRenderer::TextAlign::Center);
}

void FileBrowser::scan_directory() {
    entries_.clear();
    
    try {
        // Add parent directory entry if not at root
        fs::path current_path(current_directory_);
        if (current_path.has_parent_path() && current_path != current_path.root_path()) {
            FileEntry parent;
            parent.name = "..";
            parent.full_path = current_path.parent_path().string();
            parent.is_directory = true;
            parent.size = 0;
            entries_.push_back(parent);
        }
        
        // Scan directory entries
        std::vector<FileEntry> directories;
        std::vector<FileEntry> files;
        
        for (const auto& entry : fs::directory_iterator(current_directory_)) {
            FileEntry file_entry;
            file_entry.name = entry.path().filename().string();
            file_entry.full_path = entry.path().string();
            file_entry.is_directory = entry.is_directory();
            file_entry.size = entry.is_regular_file() ? entry.file_size() : 0;
            
            if (file_entry.is_directory) {
                directories.push_back(file_entry);
            } else if (matches_filter(file_entry.name)) {
                files.push_back(file_entry);
            }
        }
        
        // Sort directories and files alphabetically
        auto compare_names = [](const FileEntry& a, const FileEntry& b) {
            return a.name < b.name;
        };
        std::sort(directories.begin(), directories.end(), compare_names);
        std::sort(files.begin(), files.end(), compare_names);
        
        // Add directories first, then files
        entries_.insert(entries_.end(), directories.begin(), directories.end());
        entries_.insert(entries_.end(), files.begin(), files.end());
        
    } catch (const fs::filesystem_error&) {
        // If directory scan fails, add error entry
        FileEntry error;
        error.name = "[Error reading directory]";
        error.full_path = "";
        error.is_directory = false;
        error.size = 0;
        entries_.push_back(error);
    }
    
    // Reset selection if out of bounds
    if (selected_index_ >= static_cast<int>(entries_.size())) {
        selected_index_ = entries_.empty() ? 0 : static_cast<int>(entries_.size()) - 1;
    }
}

bool FileBrowser::matches_filter(const std::string& filename) const {
    if (extensions_filter_.empty()) return true;
    
    // Convert filename to lowercase for case-insensitive comparison
    std::string lower_filename = filename;
    std::transform(lower_filename.begin(), lower_filename.end(), lower_filename.begin(), ::tolower);
    
    // Split extensions filter by comma or semicolon
    std::string filter = extensions_filter_;
    std::transform(filter.begin(), filter.end(), filter.begin(), ::tolower);
    
    size_t pos = 0;
    while (pos < filter.length()) {
        size_t next = filter.find_first_of(",;", pos);
        if (next == std::string::npos) next = filter.length();
        
        std::string ext = filter.substr(pos, next - pos);
        // Trim whitespace
        ext.erase(0, ext.find_first_not_of(" \t"));
        ext.erase(ext.find_last_not_of(" \t") + 1);
        
        // Add leading dot if not present
        if (!ext.empty() && ext[0] != '.') {
            ext = "." + ext;
        }
        
        // Check if filename ends with this extension
        if (lower_filename.length() >= ext.length() &&
            lower_filename.compare(lower_filename.length() - ext.length(), ext.length(), ext) == 0) {
            return true;
        }
        
        pos = next + 1;
    }
    
    return false;
}

void FileBrowser::navigate_up() {
    fs::path current_path(current_directory_);
    if (current_path.has_parent_path() && current_path != current_path.root_path()) {
        current_directory_ = current_path.parent_path().string();
        scan_directory();
        selected_index_ = 0;
        scroll_offset_ = 0;
    }
}

void FileBrowser::navigate_into(const std::string& dir_name) {
    current_directory_ += "/" + dir_name;
    scan_directory();
    selected_index_ = 0;
}

void FileBrowser::select_current_file() {
    if (entries_.empty() || selected_index_ >= static_cast<int>(entries_.size())) return;
    const auto& entry = entries_[selected_index_];
    
    if (entry.is_directory) { 
        // Navigate into directory
        if (entry.name == "..") {
            navigate_up();
        } else {
            navigate_into(entry.name);
        }
        return; 
    }
    
    // File selected - save directory to config and close
    selected_file_ = entry.full_path;
    file_selected_ = true;
    
    // Save last directory to config based on file type
    fs::path selected_path(selected_file_);
    std::string directory = selected_path.parent_path().string();
    
    switch (current_file_type_) {
        case FileType::Cartridge:
            config_->set_last_cartridge_directory(directory);
            break;
        case FileType::ROM:
            config_->set_last_rom_directory(directory);
            break;
        case FileType::Generic:
            config_->set_last_k7_directory(directory);
            break;
    }
    config_->save();
    
    close();
}
