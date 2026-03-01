#include "ui/input_mapper.h"
#include "ui/config_manager.h"

InputMapper::InputMapper() { init_default_mappings(); }

InputMapper::~InputMapper() {
    for (auto& [id, joy] : joysticks_) {
        if (joy) SDL_JoystickClose(joy);
    }
}

void InputMapper::init_default_mappings() {
    // Basic AZERTY-ish default mapping for MO5 keyboard
    // TODO: Full keyboard matrix mapping
    keyboard_mappings_[crayon::MO5Key::ENTER] = SDLK_RETURN;
    keyboard_mappings_[crayon::MO5Key::STOP] = SDLK_ESCAPE;
    keyboard_mappings_[crayon::MO5Key::SHIFT] = SDLK_LSHIFT;
}

void InputMapper::set_keyboard_mapping(crayon::MO5Key mo5_key, SDL_Keycode host_key) {
    keyboard_mappings_[mo5_key] = host_key;
}

SDL_Keycode InputMapper::get_keyboard_mapping(crayon::MO5Key mo5_key) const {
    auto it = keyboard_mappings_.find(mo5_key);
    return (it != keyboard_mappings_.end()) ? it->second : SDLK_UNKNOWN;
}

void InputMapper::detect_joysticks() {
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
            joystick_info_[id] = {id, SDL_JoystickName(joy) ? SDL_JoystickName(joy) : "Unknown"};
        }
    }
}

std::vector<JoystickInfo> InputMapper::get_connected_joysticks() const {
    std::vector<JoystickInfo> result;
    for (const auto& [id, info] : joystick_info_) result.push_back(info);
    return result;
}

void InputMapper::load_from_config(ConfigManager& /*config*/) {
    // TODO: Load key mappings from config
}

void InputMapper::save_to_config(ConfigManager& /*config*/) {
    // TODO: Save key mappings to config
}
