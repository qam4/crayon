#ifndef CRAYON_LIGHT_PEN_H
#define CRAYON_LIGHT_PEN_H

#include "types.h"
#include <cstdint>

namespace crayon {

struct LightPenState {
    int16_t screen_x = -1;
    int16_t screen_y = -1;
    bool detected = false;
    bool button_pressed = false;
    bool strobe_active = false;
};

class LightPen {
public:
    LightPen();
    ~LightPen() = default;

    void reset();
    void set_mouse_position(int x, int y, int window_w, int window_h);
    void set_button_pressed(bool pressed);
    void update(uint16_t beam_x, uint16_t beam_y);

    bool strobe_active() const;
    bool button_pressed() const;
    uint16_t get_screen_x() const;
    uint16_t get_screen_y() const;
    bool is_detected() const;

    LightPenState get_state() const;
    void set_state(const LightPenState& state);

private:
    LightPenState state_;
};

} // namespace crayon

#endif // CRAYON_LIGHT_PEN_H
