#include "light_pen.h"
#include "types.h"

namespace crayon {

LightPen::LightPen() { reset(); }

void LightPen::reset() { state_ = LightPenState{}; }

void LightPen::set_mouse_position(int x, int y, int window_w, int window_h) {
    if (window_w > 0 && window_h > 0) {
        state_.screen_x = static_cast<int16_t>((x * DISPLAY_WIDTH) / window_w);
        state_.screen_y = static_cast<int16_t>((y * DISPLAY_HEIGHT) / window_h);
    }
}

void LightPen::set_button_pressed(bool pressed) { state_.button_pressed = pressed; }

void LightPen::update(uint16_t beam_x, uint16_t beam_y) {
    if (state_.screen_x < 0 || state_.screen_y < 0) {
        state_.detected = false;
        state_.strobe_active = false;
        return;
    }
    // Light pen detects when beam passes over pen position
    int dx = static_cast<int>(beam_x) - state_.screen_x;
    int dy = static_cast<int>(beam_y) - state_.screen_y;
    state_.detected = (dx >= -4 && dx <= 4 && dy >= -1 && dy <= 1);
    state_.strobe_active = state_.detected;
}

bool LightPen::strobe_active() const { return state_.strobe_active; }
bool LightPen::button_pressed() const { return state_.button_pressed; }
uint16_t LightPen::get_screen_x() const { return state_.screen_x >= 0 ? state_.screen_x : 0; }
uint16_t LightPen::get_screen_y() const { return state_.screen_y >= 0 ? state_.screen_y : 0; }
bool LightPen::is_detected() const { return state_.detected; }

LightPenState LightPen::get_state() const { return state_; }
void LightPen::set_state(const LightPenState& state) { state_ = state; }

} // namespace crayon
