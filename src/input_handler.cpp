#include "input_handler.h"
#include <cstring>

namespace crayon {

InputHandler::InputHandler() {
    reset();
}

void InputHandler::reset() {
    std::memset(keys_, 0, sizeof(keys_));
    reset_joystick();
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

void InputHandler::set_joystick_direction(int port, bool up, bool down, bool left, bool right) {
    if (port < 0 || port > 1) return;
    joy_[port].up    = up;
    joy_[port].down  = down;
    joy_[port].left  = left;
    joy_[port].right = right;
}

void InputHandler::set_joystick_fire(int port, bool pressed) {
    if (port < 0 || port > 1) return;
    joy_[port].fire = pressed;
}

uint8_t InputHandler::get_joystick_port_a() const {
    uint8_t port_a = 0xFF;
    if (joy_[0].up)    port_a &= ~0x01;
    if (joy_[0].down)  port_a &= ~0x02;
    if (joy_[0].left)  port_a &= ~0x04;
    if (joy_[0].right) port_a &= ~0x08;
    if (joy_[1].up)    port_a &= ~0x10;
    if (joy_[1].down)  port_a &= ~0x20;
    if (joy_[1].left)  port_a &= ~0x40;
    if (joy_[1].right) port_a &= ~0x80;
    return port_a;
}

uint8_t InputHandler::get_joystick_port_b_fire() const {
    uint8_t fire_bits = 0xC0;
    if (joy_[0].fire) fire_bits &= ~0x40;
    if (joy_[1].fire) fire_bits &= ~0x80;
    return fire_bits;
}

void InputHandler::reset_joystick() {
    joy_[0] = {};
    joy_[1] = {};
}

InputState InputHandler::get_state() const {
    InputState state;
    std::memcpy(state.keys, keys_, sizeof(keys_));
    state.joy[0] = joy_[0];
    state.joy[1] = joy_[1];
    return state;
}

void InputHandler::set_state(const InputState& state) {
    std::memcpy(keys_, state.keys, sizeof(keys_));
    joy_[0] = state.joy[0];
    joy_[1] = state.joy[1];
}

} // namespace crayon
