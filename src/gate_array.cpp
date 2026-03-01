#include "gate_array.h"
#include "types.h"

namespace crayon {

GateArray::GateArray() { reset(); }

void GateArray::reset() { state_ = GateArrayState{}; }

void GateArray::render_frame(const uint8_t* pixel_ram, const uint8_t* color_ram) {
    if (!pixel_ram || !color_ram) return;

    // MO5 forme/fond rendering: 320x200, 8-pixel blocks
    // Each byte in pixel_ram = 8 pixels (1 bit each)
    // Each byte in color_ram = foreground (high nibble) + background (low nibble)
    for (int y = 0; y < DISPLAY_HEIGHT; ++y) {
        for (int col = 0; col < 40; ++col) {
            int offset = y * 40 + col;
            uint8_t pixels = pixel_ram[offset];
            uint8_t colors = color_ram[offset];
            uint8_t fg = (colors >> 4) & 0x0F;
            uint8_t bg = colors & 0x0F;

            for (int bit = 7; bit >= 0; --bit) {
                int x = col * 8 + (7 - bit);
                bool is_fg = (pixels >> bit) & 1;
                state_.framebuffer[y][x] = MO5_PALETTE_RGBA[is_fg ? fg : bg];
            }
        }
    }

    state_.frame_number++;
    state_.frame_complete = true;
    state_.vsync_flag = true;
}

const uint32_t* GateArray::get_framebuffer() const {
    return &state_.framebuffer[0][0];
}

bool GateArray::vsync_triggered() const { return state_.vsync_flag; }
void GateArray::clear_vsync() { state_.vsync_flag = false; }

GateArrayState GateArray::get_state() const { return state_; }
void GateArray::set_state(const GateArrayState& state) { state_ = state; }

} // namespace crayon
