#ifndef CRAYON_INPUT_HANDLER_H
#define CRAYON_INPUT_HANDLER_H

#include "types.h"
#include <cstdint>
#include <map>

namespace crayon {

// MO5 keyboard key encoding: high nibble = column (0-7), low nibble = row (0-7)
// The MO5 has a 58-key AZERTY chiclet keyboard scanned as 8 columns x 8 rows.
// Column strobe is output via PIA Port B, row sense is read via PIA Port A.
// Matrix layout based on MO5 technical documentation.
enum class MO5Key : uint8_t {
    // Column 0
    N       = 0x00,
    EFF     = 0x01,  // Backspace/Delete
    J       = 0x02,
    H       = 0x03,
    U       = 0x04,
    Y       = 0x05,
    Key7    = 0x06,
    Key6    = 0x07,

    // Column 1
    COMMA   = 0x10,  // ,<
    INS     = 0x11,
    K       = 0x12,
    G       = 0x13,
    I       = 0x14,
    T       = 0x15,
    Key8    = 0x16,
    Key5    = 0x17,

    // Column 2
    DOT     = 0x20,  // .>
    DEL     = 0x21,
    L       = 0x22,
    F       = 0x23,
    O       = 0x24,
    R       = 0x25,
    Key9    = 0x26,
    Key4    = 0x27,

    // Column 3
    AT      = 0x30,  // @^
    BASIC   = 0x31,  // BASIC key (right shift position)
    M       = 0x32,
    D       = 0x33,
    P       = 0x34,
    E       = 0x35,
    Key0    = 0x36,
    Key3    = 0x37,

    // Column 4
    SPACE   = 0x40,
    RAZ     = 0x41,  // Reset/Clear
    SLASH   = 0x42,  // /?
    S       = 0x43,
    STAR    = 0x44,  // *:
    Z       = 0x45,
    MINUS   = 0x46,  // -=
    Key2    = 0x47,

    // Column 5
    X       = 0x50,
    LEFT    = 0x51,
    ENTER   = 0x52,
    Q       = 0x53,
    ACC     = 0x54,  // Accent key
    A       = 0x55,
    PLUS    = 0x56,  // +;
    Key1    = 0x57,

    // Column 6
    W       = 0x60,
    RIGHT   = 0x61,
    UP      = 0x62,
    STOP    = 0x63,
    CNT     = 0x64,  // CTRL
    C       = 0x65,
    V       = 0x66,
    B       = 0x67,

    // Column 7
    SHIFT   = 0x70,
    DOWN    = 0x71,
    UNUSED1 = 0x72,
    UNUSED2 = 0x73,
    UNUSED3 = 0x74,
    UNUSED4 = 0x75,
    UNUSED5 = 0x76,
    UNUSED6 = 0x77,
};

struct InputState {
    bool keyboard_matrix[8][8] = {};
};

class InputHandler {
public:
    InputHandler();
    ~InputHandler() = default;

    void reset();
    void set_key_state(uint8_t row, uint8_t col, bool pressed);
    void set_key_state(MO5Key key, bool pressed);
    uint8_t read_keyboard_row(uint8_t column_strobe);

    void map_host_key(int host_key, MO5Key mo5_key);
    void process_host_key(int host_key, bool pressed);

    InputState get_state() const;
    void set_state(const InputState& state);

private:
    bool keyboard_matrix_[8][8] = {};
    std::map<int, MO5Key> key_mapping_;
    void setup_default_mapping();

    static uint8_t get_col(MO5Key key) { return (static_cast<uint8_t>(key) >> 4) & 0x07; }
    static uint8_t get_row(MO5Key key) { return static_cast<uint8_t>(key) & 0x07; }
};

} // namespace crayon

#endif // CRAYON_INPUT_HANDLER_H
