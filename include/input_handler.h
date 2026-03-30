#ifndef CRAYON_INPUT_HANDLER_H
#define CRAYON_INPUT_HANDLER_H

#include "types.h"
#include <cstdint>
#include <map>

namespace crayon {

// MO5 keyboard scanning protocol:
// The MO5 ROM writes a key number (0-57) into PIA Port B bits 1-6,
// then reads back Port B bit 7 to check if that key is pressed.
// This is NOT a matrix scan — it's a direct key number lookup.
//
// Scancodes are from Theodore emulator's libretroKeyCodeToThomsonMo5ScanCode table.
// The MO5 has 57 keys (scancodes 0x00 through 0x39).
enum class MO5Key : uint8_t {
    N       = 0x00,
    EFF     = 0x01,  // Delete key
    J       = 0x02,
    H       = 0x03,
    U       = 0x04,
    Y       = 0x05,
    Key7    = 0x06,
    Key6    = 0x07,

    COMMA   = 0x08,  // , (comma) on MO5 keyboard
    INS     = 0x09,
    K       = 0x0A,
    G       = 0x0B,
    I       = 0x0C,
    T       = 0x0D,
    Key8    = 0x0E,
    Key5    = 0x0F,

    PERIOD  = 0x10,  // . (period) on MO5 keyboard
    BACKSPACE = 0x11,  // Backspace key (< on MO5 keyboard)
    L       = 0x12,
    F       = 0x13,
    O       = 0x14,
    R       = 0x15,
    Key9    = 0x16,
    Key4    = 0x17,

    AT      = 0x18,  // @ (period key on PC maps here)
    RIGHT   = 0x19,
    M       = 0x1A,  // M on MO5 keyboard
    D       = 0x1B,
    P       = 0x1C,
    E       = 0x1D,
    Key0    = 0x1E,
    Key3    = 0x1F,

    SPACE   = 0x20,
    DOWN    = 0x21,
    B       = 0x22,
    S       = 0x23,
    DIV     = 0x24,  // / (divide) key on MO5 keyboard
    Z       = 0x25,  // MO5 AZERTY: Z key (produces Z)
    MINUS   = 0x26,
    Key2    = 0x27,

    X       = 0x28,
    LEFT    = 0x29,
    V       = 0x2A,
    Q       = 0x2B,  // MO5 AZERTY: Q key (produces Q)
    STAR    = 0x2C,  // * (multiply) key on MO5 keyboard
    A       = 0x2D,  // MO5 AZERTY: A key (produces A)
    PLUS    = 0x2E,  // + (equals on PC)
    Key1    = 0x2F,

    W       = 0x30,  // MO5 AZERTY: W key (produces W)
    UP      = 0x31,
    C       = 0x32,
    RAZ     = 0x33,  // RAZ (Home/Clear) key on MO5 keyboard
    ENTER   = 0x34,
    CNT     = 0x35,  // CTRL
    ACC     = 0x36,  // ACC (accent) key on MO5 keyboard
    STOP    = 0x37,  // STOP (tab on PC)

    SHIFT   = 0x38,  // Yellow/Left shift key
    BASIC   = 0x39,  // BASIC key (right shift)
};

// Maximum number of keys (scancodes 0x00-0x39)
static constexpr int MO5_KEY_COUNT = 58;

// Joystick state for one port — true means pressed, false means released.
// Active-low encoding is applied only at the PIA read boundary.
struct JoystickState {
    bool up    = false;
    bool down  = false;
    bool left  = false;
    bool right = false;
    bool fire  = false;
};

struct InputState {
    bool keys[MO5_KEY_COUNT] = {};
    JoystickState joy[2] = {};
};

class InputHandler {
public:
    InputHandler();
    ~InputHandler() = default;

    void reset();
    void set_key_state(MO5Key key, bool pressed);

    // MO5 keyboard scan: given the PIA Port B output latch value,
    // return 0x00 if the selected key is pressed, 0x80 if not.
    // The key scancode is (port_b & 0xFE) >> 1.
    uint8_t read_key_state(uint8_t port_b_latch) const;

    void map_host_key(int host_key, MO5Key mo5_key);
    void process_host_key(int host_key, bool pressed);

    InputState get_state() const;
    void set_state(const InputState& state);

    // Joystick input — port is 0 (joy1) or 1 (joy2)
    void set_joystick_direction(int port, bool up, bool down, bool left, bool right);
    void set_joystick_fire(int port, bool pressed);

    // Read combined Port A byte (bits 0-3 = port1, bits 4-7 = port2, active low)
    uint8_t get_joystick_port_a() const;

    // Read fire button bits for Port B (bit 6 = port1 fire, bit 7 = port2 fire, active low)
    uint8_t get_joystick_port_b_fire() const;

    // Clear all joystick state to released
    void reset_joystick();

private:
    bool keys_[MO5_KEY_COUNT] = {};
    JoystickState joy_[2] = {};
    std::map<int, MO5Key> key_mapping_;
    void setup_default_mapping();
};

} // namespace crayon

#endif // CRAYON_INPUT_HANDLER_H
