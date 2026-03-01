#include "ui/file_browser.h"
#include "ui/text_renderer.h"
#include "ui/config_manager.h"

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
    scan_directory();
}

void FileBrowser::close() { is_open_ = false; }

bool FileBrowser::process_input(SDL_Keycode key) {
    if (!is_open_) return false;
    switch (key) {
        case SDLK_UP: if (selected_index_ > 0) selected_index_--; return true;
        case SDLK_DOWN: if (selected_index_ < static_cast<int>(entries_.size()) - 1) selected_index_++; return true;
        case SDLK_RETURN: select_current_file(); return true;
        case SDLK_BACKSPACE: navigate_up(); return true;
        case SDLK_ESCAPE: close(); return true;
    }
    return false;
}

void FileBrowser::render() {
    if (!is_open_) return;
    // TODO: Render file list with current directory and selection highlight
}

void FileBrowser::scan_directory() {
    entries_.clear();
    // TODO: Platform-specific directory scanning
}

bool FileBrowser::matches_filter(const std::string& filename) const {
    if (extensions_filter_.empty()) return true;
    // TODO: Check file extension against filter
    (void)filename;
    return true;
}

void FileBrowser::navigate_up() {
    // TODO: Navigate to parent directory
}

void FileBrowser::navigate_into(const std::string& dir_name) {
    current_directory_ += "/" + dir_name;
    scan_directory();
    selected_index_ = 0;
}

void FileBrowser::select_current_file() {
    if (entries_.empty() || selected_index_ >= static_cast<int>(entries_.size())) return;
    const auto& entry = entries_[selected_index_];
    if (entry.is_directory) { navigate_into(entry.name); return; }
    selected_file_ = entry.full_path;
    file_selected_ = true;
    close();
}
