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

    M       = 0x08,  // , on Thomson keyboard
    INS     = 0x09,
    K       = 0x0A,
    G       = 0x0B,
    I       = 0x0C,
    T       = 0x0D,
    Key8    = 0x0E,
    Key5    = 0x0F,

    COMMA   = 0x10,  // . on Thomson keyboard
    RAZ     = 0x11,  // Home/Reset
    L       = 0x12,
    F       = 0x13,
    O       = 0x14,
    R       = 0x15,
    Key9    = 0x16,
    Key4    = 0x17,

    AT      = 0x18,  // @ (period key on PC maps here)
    RIGHT   = 0x19,
    SLASH   = 0x1A,  // M on Thomson keyboard / semicolon on PC
    D       = 0x1B,
    P       = 0x1C,
    E       = 0x1D,
    Key0    = 0x1E,
    Key3    = 0x1F,

    SPACE   = 0x20,
    DOWN    = 0x21,
    B       = 0x22,
    S       = 0x23,
    STAR    = 0x24,  // * (left bracket on PC)
    W       = 0x25,
    MINUS   = 0x26,
    Key2    = 0x27,

    X       = 0x28,
    LEFT    = 0x29,
    V       = 0x2A,
    A       = 0x2B,
    ACC     = 0x2C,  // Accent / * key
    Q       = 0x2D,
    PLUS    = 0x2E,  // + (equals on PC)
    Key1    = 0x2F,

    Z       = 0x30,
    UP      = 0x31,
    C       = 0x32,
    DOT     = 0x33,  // RAZ on alt mapping; also used for dot
    ENTER   = 0x34,
    CNT     = 0x35,  // CTRL
    ACC2    = 0x36,  // ACC (backspace on PC)
    STOP    = 0x37,  // STOP (tab on PC)

    SHIFT   = 0x38,  // Yellow/Left shift key
    BASIC   = 0x39,  // BASIC key (right shift)
};

// Maximum number of keys (scancodes 0x00-0x39)
static constexpr int MO5_KEY_COUNT = 58;

struct InputState {
    bool keys[MO5_KEY_COUNT] = {};
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

private:
    bool keys_[MO5_KEY_COUNT] = {};
    std::map<int, MO5Key> key_mapping_;
    void setup_default_mapping();
};

} // namespace crayon

#endif // CRAYON_INPUT_HANDLER_H
