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
void MenuSystem::hide() { visible_ = false; while (!menu_stack_.empty()) menu_stack_.pop(); current_menu_ = &main_menu_; }

crayon::MenuAction MenuSystem::process_input(SDL_Keycode key) {
    if (!visible_) return crayon::MenuAction::None;
    switch (key) {
        case SDLK_UP: navigate_up(); break;
        case SDLK_DOWN: navigate_down(); break;
        case SDLK_RETURN: select_current(); break;
        case SDLK_ESCAPE: go_back(); break;
    }
    return crayon::MenuAction::None;
}

int MenuSystem::get_selected_slot() const { return last_selected_slot_; }

void MenuSystem::render() {
    if (!visible_ || !current_menu_) return;
    // TODO: Render menu items with selection highlight
}

void MenuSystem::build_main_menu() {
    main_menu_.clear();
    main_menu_.emplace_back("Load BASIC ROM", crayon::MenuAction::LoadBasicROM);
    main_menu_.emplace_back("Load Monitor ROM", crayon::MenuAction::LoadMonitorROM);
    main_menu_.emplace_back("Load Cartridge", crayon::MenuAction::LoadCartridge);
    main_menu_.emplace_back("Reset", crayon::MenuAction::Reset);
    main_menu_.emplace_back("Save State", crayon::MenuAction::SaveState);
    main_menu_.emplace_back("Load State", crayon::MenuAction::LoadState);
    main_menu_.emplace_back("Screenshot", crayon::MenuAction::Screenshot);
    main_menu_.emplace_back("Toggle Debugger", crayon::MenuAction::ToggleDebugger);
    main_menu_.emplace_back("Quit", crayon::MenuAction::Quit);
}

void MenuSystem::update_menu_values(ConfigManager*) {
    // TODO: Update menu items with current config values
}

void MenuSystem::navigate_up() { if (selected_index_ > 0) selected_index_--; }
void MenuSystem::navigate_down() {
    if (current_menu_ && selected_index_ < static_cast<int>(current_menu_->size()) - 1) selected_index_++;
}

void MenuSystem::select_current() {
    if (!current_menu_ || current_menu_->empty()) return;
    auto& item = (*current_menu_)[selected_index_];
    if (item.has_submenu && !item.submenu.empty()) {
        menu_stack_.push(current_menu_);
        current_menu_ = &item.submenu;
        selected_index_ = 0;
    } else {
        last_selected_slot_ = item.slot_number;
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
