#ifndef CRAYON_VKEYBOARD_H
#define CRAYON_VKEYBOARD_H

#include "input_handler.h"
#include "types.h"

namespace crayon {

enum class VKBPosition { Bottom, Top };
enum class VKBTransparency { Opaque, SemiTransparent, Transparent };

struct VKBKey {
    const char* label;
    int x, y, width, height;
    MO5Key mo5_key;
    int nav_up, nav_down, nav_left, nav_right;
};

enum class Direction { Up, Down, Left, Right };

namespace vkb_colors {
    constexpr uint32_t PANEL       = 0x00181818;
    constexpr uint32_t KEY_FACE    = 0x00606060;
    constexpr uint32_t KEY_SHIFT   = 0x00CCCC00;
    constexpr uint32_t KEY_BASIC   = 0x00101010;
    constexpr uint32_t BORDER      = 0x00404040;
    constexpr uint32_t CURSOR      = 0x00FFCC00;
    constexpr uint32_t TEXT        = 0x00FFFFFF;
    constexpr uint32_t CURSOR_TEXT = 0x00000000;
}

class VirtualKeyboard {
public:
    VirtualKeyboard();

    // Visibility
    void toggle_visible();
    bool is_visible() const;

    // Navigation
    void move_cursor(Direction dir);
    MO5Key press_selected() const;
    int get_cursor_index() const;

    // Touch/pointer
    int hit_test(int x, int y, int fb_height) const;
    MO5Key get_key_at(int index) const;
    void set_cursor_index(int index);

    // Modifiers
    void toggle_shift();
    bool is_shift_active() const;
    void toggle_basic();
    bool is_basic_active() const;
    void toggle_acc();
    bool is_acc_active() const;
    void toggle_cnt();
    bool is_cnt_active() const;
    MO5Key active_modifier() const;  // Returns the active modifier key, or SPACE if none

    // Position
    void toggle_position();
    void set_position(VKBPosition pos);
    VKBPosition get_position() const;

    // Transparency
    void set_transparency(VKBTransparency t);
    VKBTransparency get_transparency() const;

    // Rendering
    void render(uint32_t* framebuffer, int fb_width, int fb_height) const;

    // Layout access
    static constexpr int KEY_COUNT = 58;
    static const VKBKey LAYOUT[KEY_COUNT];

    // VKB dimensions
    static constexpr int VKB_WIDTH = 312;
    static constexpr int VKB_HEIGHT = 93;

private:
    int cursor_index_ = 0;
    bool visible_ = false;
    bool shift_active_ = false;
    bool basic_active_ = false;
    bool acc_active_ = false;
    bool cnt_active_ = false;
    VKBPosition position_ = VKBPosition::Bottom;
    VKBTransparency transparency_ = VKBTransparency::Opaque;

    int get_y_offset(int fb_height) const;
    int get_x_offset(int fb_width) const;
    uint32_t blend_pixel(uint32_t bg, uint32_t fg, uint8_t alpha) const;
    void draw_rect(uint32_t* fb, int fb_w, int fb_h,
                   int x, int y, int w, int h,
                   uint32_t color, uint8_t alpha) const;
    void draw_char(uint32_t* fb, int fb_w, int fb_h,
                   int x, int y, char ch,
                   uint32_t color, uint8_t alpha) const;
    void draw_label(uint32_t* fb, int fb_w, int fb_h,
                    int x, int y, const char* text,
                    uint32_t color, uint8_t alpha) const;
};

} // namespace crayon

#endif // CRAYON_VKEYBOARD_H
