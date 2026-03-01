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

    // MO5 16-color palette — see MO5_PALETTE_RGBA in types.h for the actual values

private:
    GateArrayState state_;
};

} // namespace crayon

#endif // CRAYON_GATE_ARRAY_H
