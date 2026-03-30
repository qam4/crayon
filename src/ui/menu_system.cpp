#include "ui/menu_system.h"
#include "ui/text_renderer.h"
#include "ui/config_manager.h"
#include "ui/save_state_manager.h"
#include <ctime>
#include <algorithm>

MenuSystem::MenuSystem(SDL_Renderer* renderer, TextRenderer* text_renderer)
    : renderer_(renderer), text_renderer_(text_renderer) {
    build_main_menu();
    current_menu_ = &main_menu_;
}

MenuSystem::~MenuSystem() = default;

void MenuSystem::show() {
    visible_ = true;
    current_menu_ = &main_menu_;
    selected_index_ = 0;
    scroll_offset_ = 0;
    while (!menu_stack_.empty()) menu_stack_.pop();
}

void MenuSystem::hide() { visible_ = false; }

void MenuSystem::build_main_menu() {
    main_menu_.clear();

    main_menu_.emplace_back("Load BASIC ROM (F2)", crayon::MenuAction::LoadBasicROM);
    main_menu_.emplace_back("Load Monitor ROM (F3)", crayon::MenuAction::LoadMonitorROM);
    main_menu_.emplace_back("Load Cartridge (F4)", crayon::MenuAction::LoadCartridge);
    main_menu_.emplace_back("Load K7 Cassette (F6)", crayon::MenuAction::LoadK7);
    main_menu_.emplace_back("Reset (F7)", crayon::MenuAction::Reset);
    main_menu_.emplace_back("Pause/Resume (F8)", crayon::MenuAction::Pause);

    // Save state submenu
    MenuItem save_menu("Save State (F9=Slot 0)", crayon::MenuAction::None);
    save_menu.has_submenu = true;
    for (int i = 0; i < 10; ++i) {
        MenuItem slot("Slot " + std::to_string(i), crayon::MenuAction::SaveState);
        slot.slot_number = i;
        save_menu.submenu.push_back(slot);
    }
    main_menu_.push_back(save_menu);

    // Load state submenu
    MenuItem load_menu("Load State (F10=Slot 0)", crayon::MenuAction::None);
    load_menu.has_submenu = true;
    for (int i = 0; i < 10; ++i) {
        MenuItem slot("Slot " + std::to_string(i), crayon::MenuAction::LoadState);
        slot.slot_number = i;
        load_menu.submenu.push_back(slot);
    }
    main_menu_.push_back(load_menu);

    // Video Settings submenu
    MenuItem video_menu("Video Settings", crayon::MenuAction::VideoSettings);
    video_menu.has_submenu = true;
    {
        MenuItem scaling("Scaling Filter", crayon::MenuAction::None);
        scaling.has_submenu = true;
        scaling.submenu.emplace_back("Nearest", crayon::MenuAction::ScalingFilterNearest);
        scaling.submenu.emplace_back("Linear", crayon::MenuAction::ScalingFilterLinear);
        video_menu.submenu.push_back(scaling);

        MenuItem aspect("Aspect Ratio", crayon::MenuAction::None);
        aspect.has_submenu = true;
        aspect.submenu.emplace_back("Original", crayon::MenuAction::AspectRatioOriginal);
        aspect.submenu.emplace_back("4:3", crayon::MenuAction::AspectRatio4_3);
        aspect.submenu.emplace_back("Stretch", crayon::MenuAction::AspectRatioStretch);
        video_menu.submenu.push_back(aspect);
    }
    main_menu_.push_back(video_menu);

    // Audio Settings submenu
    MenuItem audio_menu("Audio Settings", crayon::MenuAction::AudioSettings);
    audio_menu.has_submenu = true;
    {
        MenuItem volume("Volume", crayon::MenuAction::None);
        volume.has_submenu = true;
        const crayon::MenuAction vol_actions[] = {
            crayon::MenuAction::Volume0, crayon::MenuAction::Volume10,
            crayon::MenuAction::Volume20, crayon::MenuAction::Volume30,
            crayon::MenuAction::Volume40, crayon::MenuAction::Volume50,
            crayon::MenuAction::Volume60, crayon::MenuAction::Volume70,
            crayon::MenuAction::Volume80, crayon::MenuAction::Volume90,
            crayon::MenuAction::Volume100
        };
        for (int i = 0; i <= 10; ++i) {
            volume.submenu.emplace_back(std::to_string(i * 10) + "%", vol_actions[i]);
        }
        audio_menu.submenu.push_back(volume);
        audio_menu.submenu.emplace_back("Mute", crayon::MenuAction::ToggleMute);
    }
    main_menu_.push_back(audio_menu);

    // Input Settings submenu
    MenuItem input_menu("Input Settings", crayon::MenuAction::InputSettings);
    input_menu.has_submenu = true;
    input_menu.submenu.emplace_back("Swap Joystick Ports", crayon::MenuAction::SwapJoysticks);
    input_menu.submenu.emplace_back("Input Mapping (Ctrl+M)", crayon::MenuAction::InputMapping);
    main_menu_.push_back(input_menu);

    main_menu_.emplace_back("Screenshot (F11)", crayon::MenuAction::Screenshot);
    main_menu_.emplace_back("Toggle FPS (F12)", crayon::MenuAction::ToggleFPS);
    main_menu_.emplace_back("Toggle Fullscreen (Alt+Enter)", crayon::MenuAction::ToggleFullscreen);
    main_menu_.emplace_back("Toggle Debugger (F5)", crayon::MenuAction::ToggleDebugger);
    main_menu_.emplace_back("Quit", crayon::MenuAction::Quit);

    current_menu_ = &main_menu_;
}

void MenuSystem::navigate_up() {
    if (selected_index_ > 0) {
        selected_index_--;
        if (selected_index_ < scroll_offset_)
            scroll_offset_ = selected_index_;
    }
}

void MenuSystem::navigate_down() {
    if (!current_menu_) return;
    if (selected_index_ < static_cast<int>(current_menu_->size()) - 1) {
        selected_index_++;

        int screen_height;
        SDL_GetRendererOutputSize(renderer_, nullptr, &screen_height);
        int usable_height = screen_height - 20;  // status bar
        int margin = usable_height / 24;
        int title_height = usable_height / 15;
        int hint_height = usable_height / 20;
        int line_height = usable_height / 25;
        int available = usable_height - margin * 2 - title_height - hint_height;
        int max_visible = available / line_height;

        if (selected_index_ >= scroll_offset_ + max_visible)
            scroll_offset_ = selected_index_ - max_visible + 1;
    }
}

void MenuSystem::select_current() {
    if (!current_menu_ || current_menu_->empty()) return;
    auto& item = (*current_menu_)[selected_index_];
    if (!item.enabled) return;
    if (item.has_submenu && !item.submenu.empty()) {
        menu_stack_.push(current_menu_);
        current_menu_ = &item.submenu;
        selected_index_ = 0;
        scroll_offset_ = 0;
    }
}

void MenuSystem::go_back() {
    if (!menu_stack_.empty()) {
        current_menu_ = menu_stack_.top();
        menu_stack_.pop();
        selected_index_ = 0;
        scroll_offset_ = 0;
    } else {
        hide();
    }
}

crayon::MenuAction MenuSystem::process_input(SDL_Keycode key) {
    if (!visible_) return crayon::MenuAction::None;

    switch (key) {
        case SDLK_UP: navigate_up(); return crayon::MenuAction::None;
        case SDLK_DOWN: navigate_down(); return crayon::MenuAction::None;
        case SDLK_RETURN:
        case SDLK_SPACE:
            if (current_menu_ && !current_menu_->empty()) {
                auto& item = (*current_menu_)[selected_index_];
                if (item.has_submenu) {
                    select_current();
                    return crayon::MenuAction::None;
                } else {
                    last_selected_slot_ = item.slot_number;
                    hide();
                    return item.action;
                }
            }
            return crayon::MenuAction::None;
        case SDLK_ESCAPE:
        case SDLK_BACKSPACE:
            go_back();
            return crayon::MenuAction::None;
        default:
            return crayon::MenuAction::None;
    }
}

int MenuSystem::get_selected_slot() const { return last_selected_slot_; }

void MenuSystem::render() {
    if (!visible_ || !current_menu_) return;

    int screen_width, screen_height;
    SDL_GetRendererOutputSize(renderer_, &screen_width, &screen_height);

    // Account for the 20px status bar at the bottom
    static constexpr int STATUS_BAR_HEIGHT = 20;
    int usable_height = screen_height - STATUS_BAR_HEIGHT;

    // Semi-transparent overlay (only over the emulator area, not the status bar)
    SDL_SetRenderDrawBlendMode(renderer_, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(renderer_, 0, 0, 0, 180);
    SDL_Rect overlay = {0, 0, screen_width, usable_height};
    SDL_RenderFillRect(renderer_, &overlay);

    // Menu box — fill most of the usable area with proportional margins
    int margin = usable_height / 24;
    int menu_width = screen_width - margin * 2;
    int menu_height = usable_height - margin * 2;
    int menu_x = margin;
    int menu_y = margin;

    SDL_SetRenderDrawColor(renderer_, 40, 40, 40, 255);
    SDL_Rect menu_box = {menu_x, menu_y, menu_width, menu_height};
    SDL_RenderFillRect(renderer_, &menu_box);
    SDL_SetRenderDrawColor(renderer_, 200, 200, 200, 255);
    SDL_RenderDrawRect(renderer_, &menu_box);

    // Title
    SDL_Color title_color = {255, 255, 255, 255};
    std::string title = menu_stack_.empty() ? "Crayon Menu (F1)" : "Menu";
    int pad = screen_height / 80;
    text_renderer_->render_text(title, menu_x + pad, menu_y + pad,
                                title_color, TextRenderer::FontSize::Large);

    // Menu items
    render_menu_list(*current_menu_, selected_index_);

    // Hints
    SDL_Color hint_color = {150, 150, 150, 255};
    std::string hint = menu_stack_.empty()
        ? "Arrows: Move | Enter: Select | Esc: Close"
        : "Arrows: Move | Enter: Select | Esc: Back";
    int hint_pad = screen_height / 60;
    text_renderer_->render_text(hint, menu_x + hint_pad, menu_y + menu_height - hint_pad - 10,
                                hint_color, TextRenderer::FontSize::Medium);
}

void MenuSystem::render_menu_list(const std::vector<MenuItem>& items, int sel_index) {
    int screen_width, screen_height;
    SDL_GetRendererOutputSize(renderer_, &screen_width, &screen_height);

    static constexpr int STATUS_BAR_HEIGHT = 20;
    int usable_height = screen_height - STATUS_BAR_HEIGHT;

    int margin = usable_height / 24;
    int menu_width = screen_width - margin * 2;
    int menu_height = usable_height - margin * 2;
    int menu_x = margin;
    int menu_y = margin;

    int title_height = screen_height / 15;
    int hint_height = screen_height / 20;
    int line_height = screen_height / 25;
    int available = menu_height - title_height - hint_height;
    int max_visible = available / line_height;

    int start = scroll_offset_;
    int end = std::min(start + max_visible, static_cast<int>(items.size()));
    int item_y = menu_y + title_height;
    int item_pad = screen_height / 60;

    for (int i = start; i < end; ++i) {
        const auto& item = items[i];
        SDL_Color color;
        if (!item.enabled)
            color = {100, 100, 100, 255};
        else if (i == sel_index)
            color = {255, 255, 100, 255};
        else
            color = {200, 200, 200, 255};

        std::string text = item.label;
        if (item.has_submenu) text += " >";
        if (!item.value.empty()) text += ": " + item.value;

        text_renderer_->render_text(text, menu_x + item_pad, item_y,
                                    color, TextRenderer::FontSize::Medium);
        item_y += line_height;
    }

    // Scroll indicators
    SDL_Color arrow_color = {150, 150, 150, 255};
    if (scroll_offset_ > 0) {
        text_renderer_->render_text("^", menu_x + menu_width - screen_width / 40,
                                    menu_y + title_height, arrow_color, TextRenderer::FontSize::Medium);
    }
    if (end < static_cast<int>(items.size())) {
        text_renderer_->render_text("v", menu_x + menu_width - screen_width / 40,
                                    menu_y + menu_height - hint_height - screen_height / 40,
                                    arrow_color, TextRenderer::FontSize::Medium);
    }
}

void MenuSystem::update_menu_values(ConfigManager* /*config*/) {
    // Update menu items with current config values if needed
}

void MenuSystem::update_save_state_slots(SaveStateManagerUI* ssm, const std::string& game_name) {
    if (!ssm) return;

    std::string display_name = game_name;
    if (display_name.length() > 15)
        display_name = display_name.substr(0, 15) + "...";

    auto states = ssm->list_states(game_name);

    for (auto& item : main_menu_) {
        bool is_save = (item.label.find("Save State") != std::string::npos && item.has_submenu);
        bool is_load = (item.label.find("Load State") != std::string::npos && item.has_submenu);
        if (!is_save && !is_load) continue;

        for (size_t i = 0; i < item.submenu.size() && i < states.size(); ++i) {
            if (states[i].exists) {
                time_t ts = states[i].timestamp;
                struct tm* t = localtime(&ts);
                char buf[32];
                strftime(buf, sizeof(buf), "%m/%d %H:%M", t);
                item.submenu[i].value = display_name + " - " + std::string(buf);
                item.submenu[i].enabled = true;
            } else {
                item.submenu[i].value = "[Empty]";
                item.submenu[i].enabled = is_save;  // Can save to empty, can't load from empty
            }
        }
    }
}
