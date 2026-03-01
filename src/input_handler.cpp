#include "input_handler.h"
#include <cstring>

namespace crayon {

InputHandler::InputHandler() {
    reset();
    setup_default_mapping();
}

void InputHandler::reset() {
    std::memset(keyboard_matrix_, 0, sizeof(keyboard_matrix_));
}

void InputHandler::set_key_state(uint8_t row, uint8_t col, bool pressed) {
    if (row < 8 && col < 8) keyboard_matrix_[row][col] = pressed;
}

void InputHandler::set_key_state(MO5Key key, bool pressed) {
    set_key_state(get_row(key), get_col(key), pressed);
}

uint8_t InputHandler::read_keyboard_row(uint8_t column_strobe) {
    uint8_t result = 0xFF;
    for (int col = 0; col < 8; ++col) {
        if (!(column_strobe & (1 << col))) {
            for (int row = 0; row < 8; ++row) {
                if (keyboard_matrix_[row][col])
                    result &= ~(1 << row);
            }
        }
    }
    return result;
}

void InputHandler::map_host_key(int host_key, MO5Key mo5_key) {
    key_mapping_[host_key] = mo5_key;
}

void InputHandler::process_host_key(int host_key, bool pressed) {
    auto it = key_mapping_.find(host_key);
    if (it != key_mapping_.end()) set_key_state(it->second, pressed);
}

InputState InputHandler::get_state() const {
    InputState state;
    std::memcpy(state.keyboard_matrix, keyboard_matrix_, sizeof(keyboard_matrix_));
    return state;
}

void InputHandler::set_state(const InputState& state) {
    std::memcpy(keyboard_matrix_, state.keyboard_matrix, sizeof(keyboard_matrix_));
}

void InputHandler::setup_default_mapping() {
    // SDL2 keycodes: letters = lowercase ASCII, digits = '0'-'9'
    // Special keys use SDLK_* constants (SDL_SCANCODE_TO_KEYCODE macro: 0x40000000 | scancode)

    // Letter keys (AZERTY layout on MO5, mapped from QWERTY host)
    map_host_key('a', MO5Key::A);
    map_host_key('b', MO5Key::B);
    map_host_key('c', MO5Key::C);
    map_host_key('d', MO5Key::D);
    map_host_key('e', MO5Key::E);
    map_host_key('f', MO5Key::F);
    map_host_key('g', MO5Key::G);
    map_host_key('h', MO5Key::H);
    map_host_key('i', MO5Key::I);
    map_host_key('j', MO5Key::J);
    map_host_key('k', MO5Key::K);
    map_host_key('l', MO5Key::L);
    map_host_key('m', MO5Key::M);
    map_host_key('n', MO5Key::N);
    map_host_key('o', MO5Key::O);
    map_host_key('p', MO5Key::P);
    map_host_key('q', MO5Key::Q);
    map_host_key('r', MO5Key::R);
    map_host_key('s', MO5Key::S);
    map_host_key('t', MO5Key::T);
    map_host_key('u', MO5Key::U);
    map_host_key('v', MO5Key::V);
    map_host_key('w', MO5Key::W);
    map_host_key('x', MO5Key::X);
    map_host_key('y', MO5Key::Y);
    map_host_key('z', MO5Key::Z);

    // Number keys
    map_host_key('0', MO5Key::Key0);
    map_host_key('1', MO5Key::Key1);
    map_host_key('2', MO5Key::Key2);
    map_host_key('3', MO5Key::Key3);
    map_host_key('4', MO5Key::Key4);
    map_host_key('5', MO5Key::Key5);
    map_host_key('6', MO5Key::Key6);
    map_host_key('7', MO5Key::Key7);
    map_host_key('8', MO5Key::Key8);
    map_host_key('9', MO5Key::Key9);

    // Special keys — SDL2 SDLK_* constants
    // SDLK_RETURN = '\r' (13)
    map_host_key(13, MO5Key::ENTER);
    // SDLK_SPACE = ' ' (32)
    map_host_key(32, MO5Key::SPACE);
    // SDLK_BACKSPACE = '\b' (8)
    map_host_key(8, MO5Key::EFF);
    // SDLK_DELETE = 0x4000004C (SDL_SCANCODE_DELETE = 76)
    map_host_key(0x4000004C, MO5Key::DEL);
    // SDLK_INSERT = 0x40000049 (SDL_SCANCODE_INSERT = 73)
    map_host_key(0x40000049, MO5Key::INS);

    // Arrow keys
    // SDLK_UP = 0x40000052, DOWN = 0x40000051, LEFT = 0x40000050, RIGHT = 0x4000004F
    map_host_key(0x40000052, MO5Key::UP);
    map_host_key(0x40000051, MO5Key::DOWN);
    map_host_key(0x40000050, MO5Key::LEFT);
    map_host_key(0x4000004F, MO5Key::RIGHT);

    // Modifier keys
    // SDLK_LSHIFT = 0x400000E1, SDLK_RSHIFT = 0x400000E5
    map_host_key(0x400000E1, MO5Key::SHIFT);
    map_host_key(0x400000E5, MO5Key::BASIC);  // Right shift = BASIC key on MO5
    // SDLK_LCTRL = 0x400000E0
    map_host_key(0x400000E0, MO5Key::CNT);

    // Punctuation
    map_host_key(',', MO5Key::COMMA);
    map_host_key('.', MO5Key::DOT);
    map_host_key('/', MO5Key::SLASH);
    map_host_key('-', MO5Key::MINUS);
    map_host_key('=', MO5Key::PLUS);   // = key maps to +; on MO5
    map_host_key('@', MO5Key::AT);
    map_host_key('*', MO5Key::STAR);

    // Function keys mapped to MO5 special keys
    // SDLK_TAB = '\t' (9) -> ACC (accent)
    map_host_key(9, MO5Key::ACC);
    // SDLK_HOME = 0x4000004A -> RAZ (reset)
    map_host_key(0x4000004A, MO5Key::RAZ);
    // SDLK_END = 0x4000004D -> STOP
    map_host_key(0x4000004D, MO5Key::STOP);
}

} // namespace crayon
