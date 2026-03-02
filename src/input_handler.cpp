#include "input_handler.h"
#include <cstring>

namespace crayon {

InputHandler::InputHandler() {
    reset();
}

void InputHandler::reset() {
    std::memset(keys_, 0, sizeof(keys_));
}

void InputHandler::set_key_state(MO5Key key, bool pressed) {
    uint8_t scancode = static_cast<uint8_t>(key);
    if (scancode < MO5_KEY_COUNT) keys_[scancode] = pressed;
}

uint8_t InputHandler::read_key_state(uint8_t port_b_latch) const {
    uint8_t scancode = (port_b_latch & 0xFE) >> 1;
    if (scancode < MO5_KEY_COUNT && keys_[scancode])
        return 0x00;
    return 0x80;
}

InputState InputHandler::get_state() const {
    InputState state;
    std::memcpy(state.keys, keys_, sizeof(keys_));
    return state;
}

void InputHandler::set_state(const InputState& state) {
    std::memcpy(keys_, state.keys, sizeof(keys_));
}

} // namespace crayon
