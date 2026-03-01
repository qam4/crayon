#ifndef CRAYON_GATE_ARRAY_H
#define CRAYON_GATE_ARRAY_H

#include "types.h"
#include <cstdint>

namespace crayon {

struct GateArrayState {
    uint32_t framebuffer[200][320] = {};
    uint16_t beam_x = 0;
    uint16_t beam_y = 0;
    uint64_t frame_number = 0;
    bool frame_complete = false;
    bool vsync_flag = false;
    uint8_t border_color = 0;
};

class GateArray {
public:
    GateArray();
    ~GateArray() = default;

    void reset();
    void render_frame(const uint8_t* pixel_ram, const uint8_t* color_ram);

    const uint32_t* get_framebuffer() const;
    bool vsync_triggered() const;
    void clear_vsync();

    GateArrayState get_state() const;
    void set_state(const GateArrayState& state);

    // MO5 16-color palette (fixed, from design document)
    static constexpr uint32_t PALETTE[16] = {
        0x000000FF, // 0: black
        0xFF0000FF, // 1: red
        0x00FF00FF, // 2: green
        0xFFFF00FF, // 3: yellow
        0x0000FFFF, // 4: blue
        0xFF00FFFF, // 5: magenta
        0x00FFFFFF, // 6: cyan
        0xFFFFFFFF, // 7: white
        0x808080FF, // 8: grey
        0xFF8080FF, // 9: light red
        0x80FF80FF, // 10: light green
        0xFFFF80FF, // 11: light yellow
        0x8080FFFF, // 12: light blue
        0xFF80FFFF, // 13: light magenta
        0x80FFFFFF, // 14: light cyan
        0xFF8000FF  // 15: orange
    };

private:
    GateArrayState state_;
};

} // namespace crayon

#endif // CRAYON_GATE_ARRAY_H
