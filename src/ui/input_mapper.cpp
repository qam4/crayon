#include "ui/input_mapper.h"
#include "ui/config_manager.h"
#include "ui/text_renderer.h"
#include <sstream>

InputMapper::InputMapper() { 
    init_default_mappings(); 
}

InputMapper::~InputMapper() {
    for (auto& [id, joy] : joysticks_) {
        if (joy) SDL_JoystickClose(joy);
    }
}

void InputMapper::init_default_mappings() {
    // Default QWERTY keyboard mapping for MO5 keys
    keyboard_mappings_[crayon::MO5Key::ENTER] = SDLK_RETURN;
    keyboard_mappings_[crayon::MO5Key::STOP] = SDLK_ESCAPE;
    keyboard_mappings_[crayon::MO5Key::SHIFT] = SDLK_LSHIFT;
    keyboard_mappings_[crayon::MO5Key::CNT] = SDLK_LCTRL;
    keyboard_mappings_[crayon::MO5Key::ACC] = SDLK_LALT;
    keyboard_mappings_[crayon::MO5Key::UP] = SDLK_UP;
    keyboard_mappings_[crayon::MO5Key::DOWN] = SDLK_DOWN;
    keyboard_mappings_[crayon::MO5Key::LEFT] = SDLK_LEFT;
    keyboard_mappings_[crayon::MO5Key::RIGHT] = SDLK_RIGHT;
    keyboard_mappings_[crayon::MO5Key::SPACE] = SDLK_SPACE;
    
    // Alphanumeric keys
    keyboard_mappings_[crayon::MO5Key::A] = SDLK_a;
    keyboard_mappings_[crayon::MO5Key::B] = SDLK_b;
    keyboard_mappings_[crayon::MO5Key::C] = SDLK_c;
    keyboard_mappings_[crayon::MO5Key::D] = SDLK_d;
    keyboard_mappings_[crayon::MO5Key::E] = SDLK_e;
    keyboard_mappings_[crayon::MO5Key::F] = SDLK_f;
    keyboard_mappings_[crayon::MO5Key::G] = SDLK_g;
    keyboard_mappings_[crayon::MO5Key::H] = SDLK_h;
    keyboard_mappings_[crayon::MO5Key::I] = SDLK_i;
    keyboard_mappings_[crayon::MO5Key::J] = SDLK_j;
    keyboard_mappings_[crayon::MO5Key::K] = SDLK_k;
    keyboard_mappings_[crayon::MO5Key::L] = SDLK_l;
    keyboard_mappings_[crayon::MO5Key::M] = SDLK_m;
    keyboard_mappings_[crayon::MO5Key::N] = SDLK_n;
    keyboard_mappings_[crayon::MO5Key::O] = SDLK_o;
    keyboard_mappings_[crayon::MO5Key::P] = SDLK_p;
    keyboard_mappings_[crayon::MO5Key::Q] = SDLK_q;
    keyboard_mappings_[crayon::MO5Key::R] = SDLK_r;
    keyboard_mappings_[crayon::MO5Key::S] = SDLK_s;
    keyboard_mappings_[crayon::MO5Key::T] = SDLK_t;
    keyboard_mappings_[crayon::MO5Key::U] = SDLK_u;
    keyboard_mappings_[crayon::MO5Key::V] = SDLK_v;
    keyboard_mappings_[crayon::MO5Key::W] = SDLK_w;
    keyboard_mappings_[crayon::MO5Key::X] = SDLK_x;
    keyboard_mappings_[crayon::MO5Key::Y] = SDLK_y;
    keyboard_mappings_[crayon::MO5Key::Z] = SDLK_z;
    
    keyboard_mappings_[crayon::MO5Key::Key0] = SDLK_0;
    keyboard_mappings_[crayon::MO5Key::Key1] = SDLK_1;
    keyboard_mappings_[crayon::MO5Key::Key2] = SDLK_2;
    keyboard_mappings_[crayon::MO5Key::Key3] = SDLK_3;
    keyboard_mappings_[crayon::MO5Key::Key4] = SDLK_4;
    keyboard_mappings_[crayon::MO5Key::Key5] = SDLK_5;
    keyboard_mappings_[crayon::MO5Key::Key6] = SDLK_6;
    keyboard_mappings_[crayon::MO5Key::Key7] = SDLK_7;
    keyboard_mappings_[crayon::MO5Key::Key8] = SDLK_8;
    keyboard_mappings_[crayon::MO5Key::Key9] = SDLK_9;
}

void InputMapper::set_keyboard_mapping(crayon::MO5Key mo5_key, SDL_Keycode host_key) {
    keyboard_mappings_[mo5_key] = host_key;
}

SDL_Keycode InputMapper::get_keyboard_mapping(crayon::MO5Key mo5_key) const {
    auto it = keyboard_mappings_.find(mo5_key);
    return (it != keyboard_mappings_.end()) ? it->second : SDLK_UNKNOWN;
}

void InputMapper::reset_keyboard_mappings() {
    keyboard_mappings_.clear();
    init_default_mappings();
}

void InputMapper::set_joystick_button_mapping(crayon::MO5Key mo5_key, int joystick_id, int button) {
    JoystickInput input;
    input.joystick_id = joystick_id;
    input.button = button;
    input.axis = -1;
    joystick_mappings_[mo5_key] = input;
}

void InputMapper::set_joystick_axis_mapping(crayon::MO5Key mo5_key, int joystick_id, int axis, int direction) {
    JoystickInput input;
    input.joystick_id = joystick_id;
    input.button = -1;
    input.axis = axis;
    input.axis_direction = direction;
    joystick_mappings_[mo5_key] = input;
}

JoystickInput InputMapper::get_joystick_mapping(crayon::MO5Key mo5_key) const {
    auto it = joystick_mappings_.find(mo5_key);
    return (it != joystick_mappings_.end()) ? it->second : JoystickInput{};
}

void InputMapper::detect_joysticks() {
    // Close existing joysticks
    for (auto& [id, joy] : joysticks_) {
        if (joy) SDL_JoystickClose(joy);
    }
    joysticks_.clear();
    joystick_info_.clear();

    int count = SDL_NumJoysticks();
    for (int i = 0; i < count; ++i) {
        SDL_Joystick* joy = SDL_JoystickOpen(i);
        if (joy) {
            int id = SDL_JoystickInstanceID(joy);
            joysticks_[id] = joy;
            JoystickInfo info;
            info.id = id;
            info.name = SDL_JoystickName(joy) ? SDL_JoystickName(joy) : "Unknown";
            info.num_buttons = SDL_JoystickNumButtons(joy);
            info.num_axes = SDL_JoystickNumAxes(joy);
            joystick_info_[id] = info;
        }
    }
}

std::vector<JoystickInfo> InputMapper::get_connected_joysticks() const {
    std::vector<JoystickInfo> result;
    for (const auto& [id, info] : joystick_info_) {
        result.push_back(info);
    }
    return result;
}

void InputMapper::load_from_config(ConfigManager& /*config*/) {
    // TODO: Implement config loading
    // For now, use default mappings
}

void InputMapper::save_to_config(ConfigManager& /*config*/) {
    // TODO: Implement config saving
}

std::string InputMapper::mo5_key_to_string(crayon::MO5Key key) const {
    switch (key) {
        case crayon::MO5Key::ENTER: return "ENTER";
        case crayon::MO5Key::STOP: return "STOP";
        case crayon::MO5Key::SHIFT: return "SHIFT";
        case crayon::MO5Key::CNT: return "CNT";
        case crayon::MO5Key::ACC: return "ACC";
        case crayon::MO5Key::UP: return "UP";
        case crayon::MO5Key::DOWN: return "DOWN";
        case crayon::MO5Key::LEFT: return "LEFT";
        case crayon::MO5Key::RIGHT: return "RIGHT";
        case crayon::MO5Key::SPACE: return "SPACE";
        case crayon::MO5Key::A: return "A";
        case crayon::MO5Key::B: return "B";
        case crayon::MO5Key::C: return "C";
        case crayon::MO5Key::D: return "D";
        case crayon::MO5Key::E: return "E";
        case crayon::MO5Key::F: return "F";
        case crayon::MO5Key::G: return "G";
        case crayon::MO5Key::H: return "H";
        case crayon::MO5Key::I: return "I";
        case crayon::MO5Key::J: return "J";
        case crayon::MO5Key::K: return "K";
        case crayon::MO5Key::L: return "L";
        case crayon::MO5Key::M: return "M";
        case crayon::MO5Key::N: return "N";
        case crayon::MO5Key::O: return "O";
        case crayon::MO5Key::P: return "P";
        case crayon::MO5Key::Q: return "Q";
        case crayon::MO5Key::R: return "R";
        case crayon::MO5Key::S: return "S";
        case crayon::MO5Key::T: return "T";
        case crayon::MO5Key::U: return "U";
        case crayon::MO5Key::V: return "V";
        case crayon::MO5Key::W: return "W";
        case crayon::MO5Key::X: return "X";
        case crayon::MO5Key::Y: return "Y";
        case crayon::MO5Key::Z: return "Z";
        case crayon::MO5Key::Key0: return "0";
        case crayon::MO5Key::Key1: return "1";
        case crayon::MO5Key::Key2: return "2";
        case crayon::MO5Key::Key3: return "3";
        case crayon::MO5Key::Key4: return "4";
        case crayon::MO5Key::Key5: return "5";
        case crayon::MO5Key::Key6: return "6";
        case crayon::MO5Key::Key7: return "7";
        case crayon::MO5Key::Key8: return "8";
        case crayon::MO5Key::Key9: return "9";
        default: return "UNKNOWN";
    }
}

std::string InputMapper::sdl_keycode_to_string(SDL_Keycode key) const {
    const char* name = SDL_GetKeyName(key);
    return name ? std::string(name) : "UNKNOWN";
}

void InputMapper::render_mapping_ui(SDL_Renderer* renderer, TextRenderer* text_renderer) {
    if (!show_ui_) return;
    
    // Semi-transparent overlay
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 180);
    int window_width, window_height;
    SDL_GetRendererOutputSize(renderer, &window_width, &window_height);
    SDL_Rect overlay = {0, 0, window_width, window_height};
    SDL_RenderFillRect(renderer, &overlay);
    
    // Panel
    SDL_SetRenderDrawColor(renderer, 40, 40, 40, 255);
    SDL_Rect panel = {50, 30, window_width - 100, window_height - 60};
    SDL_RenderFillRect(renderer, &panel);
    SDL_SetRenderDrawColor(renderer, 100, 100, 100, 255);
    SDL_RenderDrawRect(renderer, &panel);
    
    // Title
    SDL_Color white = {255, 255, 255, 255};
    text_renderer->render_text(renderer, "Input Mapper Configuration", 
                               window_width / 2, 50, white, TextRenderer::TextAlign::Center);
    
    // Instructions
    SDL_Color gray = {180, 180, 180, 255};
    if (waiting_for_input_) {
        text_renderer->render_text(renderer, "Press a key to map...", 
                                   window_width / 2, 80, gray, TextRenderer::TextAlign::Center);
    } else {
        text_renderer->render_text(renderer, "UP/DOWN: Navigate | ENTER: Remap | R: Reset | ESC: Close", 
                                   window_width / 2, 80, gray, TextRenderer::TextAlign::Center);
    }
    
    // List some key mappings
    int y = 120;
    int index = 0;
    for (const auto& [mo5_key, host_key] : keyboard_mappings_) {
        if (index == selected_key_index_) {
            SDL_SetRenderDrawColor(renderer, 60, 60, 120, 255);
            SDL_Rect highlight = {60, y - 2, window_width - 120, 22};
            SDL_RenderFillRect(renderer, &highlight);
        }
        
        std::string mo5_name = mo5_key_to_string(mo5_key);
        std::string host_name = sdl_keycode_to_string(host_key);
        std::string line = mo5_name + " -> " + host_name;
        
        SDL_Color color = (index == selected_key_index_) ? white : gray;
        text_renderer->render_text(renderer, line.c_str(), 70, y, color, TextRenderer::TextAlign::Left);
        
        y += 24;
        index++;
        if (y > window_height - 100) break; // Don't overflow
    }
}

bool InputMapper::process_mapping_ui_input(SDL_Keycode key) {
    if (!show_ui_) return false;
    
    if (waiting_for_input_) {
        // Capture the key for remapping
        auto it = keyboard_mappings_.begin();
        std::advance(it, selected_key_index_);
        if (it != keyboard_mappings_.end()) {
            it->second = key;
        }
        waiting_for_input_ = false;
        return true;
    }
    
    switch (key) {
        case SDLK_UP:
            if (selected_key_index_ > 0) selected_key_index_--;
            return true;
        case SDLK_DOWN:
            if (selected_key_index_ < static_cast<int>(keyboard_mappings_.size()) - 1) {
                selected_key_index_++;
            }
            return true;
        case SDLK_RETURN:
            waiting_for_input_ = true;
            return true;
        case SDLK_r:
            reset_keyboard_mappings();
            return true;
        case SDLK_ESCAPE:
            show_ui_ = false;
            return true;
    }
    
    return false;
}
