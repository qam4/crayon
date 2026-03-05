#ifndef CRAYON_VKEYBOARD_H
#define CRAYON_VKEYBOARD_H

#include "input_handler.h"
#include "types.h"

namespace crayon {

enum class VKBPosition { Bottom, Top };
enum class VKBTransparency { Opaque, SemiTransparent, Transparent };

struct VKBKey {
    MO5Key mo5_key;
    const char* label;
    uint8_t row;
    uint8_t col;
    uint8_t width;  // Width in grid units (1 = normal, 2+ = wide)
};

class VirtualKeyboard {
public:
    VirtualKeyboard();

    void toggle_visible();
    bool is_visible() const;

    // Navigation
    void move_cursor(int dx, int dy);
    MO5Key press_selected();
    void toggle_shift();
    void toggle_position();

    // Configuration
    void set_position(VKBPosition pos);
    void set_transparency(VKBTransparency t);
    VKBPosition get_position() const;

    // Query
    MO5Key get_selected_key() const;
    bool is_shift_active() const;

    // Rendering — composites onto existing XRGB8888 framebuffer
    void render(uint32_t* framebuffer, int fb_width, int fb_height);

    static const VKBKey LAYOUT[];
    static const int ROW_COUNT;
    static const int MAX_COL_COUNT;

private:
    bool visible_ = false;
    bool shift_active_ = false;
    int cursor_row_ = 0;
    int cursor_col_ = 0;
    VKBPosition position_ = VKBPosition::Bottom;
    VKBTransparency transparency_ = VKBTransparency::Opaque;

    int get_key_index(int row, int col) const;
    void clamp_cursor();
    uint32_t blend_pixel(uint32_t bg, uint32_t fg, uint8_t alpha) const;
};

} // namespace crayon

#endif // CRAYON_VKEYBOARD_H
