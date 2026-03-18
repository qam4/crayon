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

    // Default joystick mappings (joystick 0):
    // Axis 0 (left stick X) → LEFT/RIGHT
    // Axis 1 (left stick Y) → UP/DOWN
    // Button 0 (A/Cross)    → SPACE (action)
    // Button 1 (B/Circle)   → ENTER
    // Button 6 (Back/Select)→ STOP
    set_joystick_axis_mapping(crayon::MO5Key::LEFT,  0, 0, -1);
    set_joystick_axis_mapping(crayon::MO5Key::RIGHT, 0, 0, +1);
    set_joystick_axis_mapping(crayon::MO5Key::UP,    0, 1, -1);
    set_joystick_axis_mapping(crayon::MO5Key::DOWN,  0, 1, +1);
    set_joystick_button_mapping(crayon::MO5Key::SPACE, 0, 0);
    set_joystick_button_mapping(crayon::MO5Key::ENTER, 0, 1);
    set_joystick_button_mapping(crayon::MO5Key::STOP,  0, 6);
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

void InputMapper::load_from_config(ConfigManager& config) {
    // Load joystick modifier key
    std::string mod_name = config.get_value("Joystick", "modifier", "RALT");
    if (mod_name == "RALT") joystick_modifier_ = SDL_SCANCODE_RALT;
    else if (mod_name == "RCTRL") joystick_modifier_ = SDL_SCANCODE_RCTRL;
    else if (mod_name == "LGUI" || mod_name == "LWIN") joystick_modifier_ = SDL_SCANCODE_LGUI;
    else if (mod_name == "RGUI" || mod_name == "RWIN") joystick_modifier_ = SDL_SCANCODE_RGUI;
    else joystick_modifier_ = SDL_SCANCODE_RALT;

    // Load joystick mappings from [Joystick] section
    // Format: joy_<mo5key>=<joystick_id>,<type>,<index>[,<direction>]
    // type: "button" or "axis"
    // Example: joy_UP=0,axis,1,-1   joy_SPACE=0,button,0
    for (int i = 0; i < crayon::MO5_KEY_COUNT; ++i) {
        auto mo5_key = static_cast<crayon::MO5Key>(i);
        std::string key_name = "joy_" + mo5_key_to_string(mo5_key);
        std::string val = config.get_value("Joystick", key_name, "");
        if (val.empty()) continue;

        std::istringstream ss(val);
        std::string token;
        std::vector<std::string> parts;
        while (std::getline(ss, token, ',')) parts.push_back(token);

        if (parts.size() < 3) continue;
        try {
            int joy_id = std::stoi(parts[0]);
            if (parts[1] == "button") {
                set_joystick_button_mapping(mo5_key, joy_id, std::stoi(parts[2]));
            } else if (parts[1] == "axis" && parts.size() >= 4) {
                set_joystick_axis_mapping(mo5_key, joy_id, std::stoi(parts[2]), std::stoi(parts[3]));
            }
        } catch (...) {
            // Skip malformed entries
        }
    }
}

void InputMapper::save_to_config(ConfigManager& config) {
    // Save joystick modifier
    std::string mod_name = "RALT";
    if (joystick_modifier_ == SDL_SCANCODE_RALT) mod_name = "RALT";
    else if (joystick_modifier_ == SDL_SCANCODE_RCTRL) mod_name = "RCTRL";
    else if (joystick_modifier_ == SDL_SCANCODE_LGUI) mod_name = "LGUI";
    else if (joystick_modifier_ == SDL_SCANCODE_RGUI) mod_name = "RGUI";
    config.set_value("Joystick", "modifier", mod_name);

    for (const auto& [mo5_key, joy] : joystick_mappings_) {
        std::string key_name = "joy_" + mo5_key_to_string(mo5_key);
        std::ostringstream val;
        if (joy.is_button()) {
            val << joy.joystick_id << ",button," << joy.button;
        } else if (joy.is_axis()) {
            val << joy.joystick_id << ",axis," << joy.axis << "," << joy.axis_direction;
        } else {
            continue;
        }
        config.set_value("Joystick", key_name, val.str());
    }
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
    SDL_Color gray = {180, 180, 180, 255};
    SDL_Color cyan = {100, 200, 255, 255};

    const char* title = show_joystick_tab_ ? "Joystick Mapping" : "Keyboard Mapping";
    text_renderer->render_text(renderer, title,
                               window_width / 2, 50, white, TextRenderer::TextAlign::Center);
    
    // Instructions
    if (waiting_for_input_) {
        const char* prompt = show_joystick_tab_
            ? "Press a joystick button or move an axis..."
            : "Press a key to map...";
        text_renderer->render_text(renderer, prompt,
                                   window_width / 2, 80, gray, TextRenderer::TextAlign::Center);
    } else {
        text_renderer->render_text(renderer, "UP/DOWN: Navigate | ENTER: Remap | TAB: Keyboard/Joystick | R: Reset | ESC: Close", 
                                   window_width / 2, 80, gray, TextRenderer::TextAlign::Center);
    }

    int y = 120;
    int index = 0;

    if (!show_joystick_tab_) {
        // Keyboard mappings
        for (const auto& [mo5_key, host_key] : keyboard_mappings_) {
            if (index == selected_key_index_) {
                SDL_SetRenderDrawColor(renderer, 60, 60, 120, 255);
                SDL_Rect highlight = {60, y - 2, window_width - 120, 22};
                SDL_RenderFillRect(renderer, &highlight);
            }
            std::string line = mo5_key_to_string(mo5_key) + " -> " + sdl_keycode_to_string(host_key);
            SDL_Color color = (index == selected_key_index_) ? white : gray;
            text_renderer->render_text(renderer, line.c_str(), 70, y, color, TextRenderer::TextAlign::Left);
            y += 24;
            index++;
            if (y > window_height - 100) break;
        }
    } else {
        // Joystick mappings
        // Show all MO5 keys that have joystick bindings, plus common game keys without bindings
        crayon::MO5Key game_keys[] = {
            crayon::MO5Key::UP, crayon::MO5Key::DOWN, crayon::MO5Key::LEFT, crayon::MO5Key::RIGHT,
            crayon::MO5Key::SPACE, crayon::MO5Key::ENTER, crayon::MO5Key::STOP,
            crayon::MO5Key::SHIFT, crayon::MO5Key::CNT
        };
        for (auto mo5_key : game_keys) {
            if (index == selected_key_index_) {
                SDL_SetRenderDrawColor(renderer, 60, 80, 60, 255);
                SDL_Rect highlight = {60, y - 2, window_width - 120, 22};
                SDL_RenderFillRect(renderer, &highlight);
            }
            auto joy = get_joystick_mapping(mo5_key);
            std::string binding = "(none)";
            if (joy.is_button()) {
                binding = "Joy" + std::to_string(joy.joystick_id) + " Btn" + std::to_string(joy.button);
            } else if (joy.is_axis()) {
                binding = "Joy" + std::to_string(joy.joystick_id) + " Axis" + std::to_string(joy.axis)
                        + (joy.axis_direction > 0 ? "+" : "-");
            }
            std::string line = mo5_key_to_string(mo5_key) + " -> " + binding;
            SDL_Color color = (index == selected_key_index_) ? white : cyan;
            text_renderer->render_text(renderer, line.c_str(), 70, y, color, TextRenderer::TextAlign::Left);
            y += 24;
            index++;
            if (y > window_height - 100) break;
        }
    }
}

bool InputMapper::process_mapping_ui_input(SDL_Keycode key) {
    if (!show_ui_) return false;
    
    if (waiting_for_input_ && !show_joystick_tab_) {
        // Capture the key for keyboard remapping
        auto it = keyboard_mappings_.begin();
        std::advance(it, selected_key_index_);
        if (it != keyboard_mappings_.end()) {
            it->second = key;
        }
        waiting_for_input_ = false;
        return true;
    }
    
    // For joystick tab, waiting_for_input_ is handled by process_joystick_mapping_input()
    
    int max_index;
    if (show_joystick_tab_) {
        max_index = 8; // 9 game keys (UP/DOWN/LEFT/RIGHT/SPACE/ENTER/STOP/SHIFT/CNT)
    } else {
        max_index = static_cast<int>(keyboard_mappings_.size()) - 1;
    }

    switch (key) {
        case SDLK_UP:
            if (selected_key_index_ > 0) selected_key_index_--;
            return true;
        case SDLK_DOWN:
            if (selected_key_index_ < max_index) selected_key_index_++;
            return true;
        case SDLK_RETURN:
            waiting_for_input_ = true;
            return true;
        case SDLK_TAB:
            show_joystick_tab_ = !show_joystick_tab_;
            selected_key_index_ = 0;
            waiting_for_input_ = false;
            return true;
        case SDLK_r:
            if (show_joystick_tab_) {
                joystick_mappings_.clear();
                // Re-apply defaults
                set_joystick_axis_mapping(crayon::MO5Key::LEFT,  0, 0, -1);
                set_joystick_axis_mapping(crayon::MO5Key::RIGHT, 0, 0, +1);
                set_joystick_axis_mapping(crayon::MO5Key::UP,    0, 1, -1);
                set_joystick_axis_mapping(crayon::MO5Key::DOWN,  0, 1, +1);
                set_joystick_button_mapping(crayon::MO5Key::SPACE, 0, 0);
                set_joystick_button_mapping(crayon::MO5Key::ENTER, 0, 1);
                set_joystick_button_mapping(crayon::MO5Key::STOP,  0, 6);
            } else {
                reset_keyboard_mappings();
            }
            return true;
        case SDLK_ESCAPE:
            show_ui_ = false;
            return true;
    }
    
    return false;
}

bool InputMapper::process_joystick_mapping_input(const SDL_Event& event) {
    if (!show_ui_ || !show_joystick_tab_ || !waiting_for_input_) return false;

    // Map selected_key_index_ back to the MO5Key
    crayon::MO5Key game_keys[] = {
        crayon::MO5Key::UP, crayon::MO5Key::DOWN, crayon::MO5Key::LEFT, crayon::MO5Key::RIGHT,
        crayon::MO5Key::SPACE, crayon::MO5Key::ENTER, crayon::MO5Key::STOP,
        crayon::MO5Key::SHIFT, crayon::MO5Key::CNT
    };
    if (selected_key_index_ < 0 || selected_key_index_ >= 9) return false;
    crayon::MO5Key target = game_keys[selected_key_index_];

    if (event.type == SDL_JOYBUTTONDOWN) {
        set_joystick_button_mapping(target, event.jbutton.which, event.jbutton.button);
        waiting_for_input_ = false;
        return true;
    }
    if (event.type == SDL_JOYAXISMOTION) {
        // Only capture if axis is pushed past deadzone
        if (event.jaxis.value > 16000) {
            set_joystick_axis_mapping(target, event.jaxis.which, event.jaxis.axis, +1);
            waiting_for_input_ = false;
            return true;
        } else if (event.jaxis.value < -16000) {
            set_joystick_axis_mapping(target, event.jaxis.which, event.jaxis.axis, -1);
            waiting_for_input_ = false;
            return true;
        }
    }
    return false;
}
