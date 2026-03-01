#include "input_handler.h"
#include <cstring>

namespace crayon {

InputHandler::InputHandler() { reset(); }

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

} // namespace crayon
