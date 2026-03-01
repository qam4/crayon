#ifndef CRAYON_INPUT_HANDLER_H
#define CRAYON_INPUT_HANDLER_H

#include "types.h"
#include <cstdint>
#include <map>

namespace crayon {

// MO5 keyboard key encoding: high nibble = row, low nibble = column
enum class MO5Key : uint8_t {
    // Placeholder — full matrix layout filled in during Milestone 6
    N = 0x00, EFF = 0x01, J = 0x02, H = 0x03,
    U = 0x04, Y = 0x05, Key7 = 0x06, Key6 = 0x07,
    SHIFT = 0x70, STOP = 0x71, ENTER = 0x77,
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

    static uint8_t get_row(MO5Key key) { return (static_cast<uint8_t>(key) >> 4) & 0x07; }
    static uint8_t get_col(MO5Key key) { return static_cast<uint8_t>(key) & 0x07; }
};

} // namespace crayon

#endif // CRAYON_INPUT_HANDLER_H
