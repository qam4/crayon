#include "vkeyboard.h"
#include "vkeyboard_font.h"
#include <algorithm>
#include <cstring>

namespace crayon {

// Complete 58-key layout for the Thomson MO5 virtual keyboard.
// Each entry: { label, x, y, width, height, mo5_key, nav_up, nav_down, nav_left, nav_right }
// Coordinates are relative to VKB top-left (0,0).
// Nav links: index into LAYOUT[], or -1 for no neighbor.
const VKBKey VirtualKeyboard::LAYOUT[KEY_COUNT] = {
    // Row 0 (15 keys): y=2, h=17
    /*  0 */ { "STP",   2,  2, 20, 17, MO5Key::STOP,   -1, 15,  -1,   1 },
    /*  1 */ { "1",    22,  2, 20, 17, MO5Key::Key1,   -1, 16,   0,   2 },
    /*  2 */ { "2",    42,  2, 20, 17, MO5Key::Key2,   -1, 17,   1,   3 },
    /*  3 */ { "3",    62,  2, 20, 17, MO5Key::Key3,   -1, 18,   2,   4 },
    /*  4 */ { "4",    82,  2, 20, 17, MO5Key::Key4,   -1, 19,   3,   5 },
    /*  5 */ { "5",   102,  2, 20, 17, MO5Key::Key5,   -1, 20,   4,   6 },
    /*  6 */ { "6",   122,  2, 20, 17, MO5Key::Key6,   -1, 21,   5,   7 },
    /*  7 */ { "7",   142,  2, 20, 17, MO5Key::Key7,   -1, 22,   6,   8 },
    /*  8 */ { "8",   162,  2, 20, 17, MO5Key::Key8,   -1, 23,   7,   9 },
    /*  9 */ { "9",   182,  2, 20, 17, MO5Key::Key9,   -1, 24,   8,  10 },
    /* 10 */ { "0",   202,  2, 20, 17, MO5Key::Key0,   -1, 25,   9,  11 },
    /* 11 */ { "-",   222,  2, 20, 17, MO5Key::MINUS,  -1, 26,  10,  12 },
    /* 12 */ { "+",   242,  2, 20, 17, MO5Key::PLUS,   -1, 27,  11,  13 },
    /* 13 */ { "ACC", 262,  2, 20, 17, MO5Key::ACC,    -1, 28,  12,  14 },
    /* 14 */ { "^",   282,  2, 20, 17, MO5Key::UP,     -1, 29,  13,  -1 },

    // Row 1 (15 keys): y=20, h=17 — 10px gap after CNT
    /* 15 */ { "CNT",   2, 20, 20, 17, MO5Key::CNT,     0, 30,  -1,  16 },
    /* 16 */ { "A",    32, 20, 20, 17, MO5Key::A,       1, 32,  15,  17 },
    /* 17 */ { "Z",    52, 20, 20, 17, MO5Key::Z,       2, 33,  16,  18 },
    /* 18 */ { "E",    72, 20, 20, 17, MO5Key::E,       3, 34,  17,  19 },
    /* 19 */ { "R",    92, 20, 20, 17, MO5Key::R,       4, 35,  18,  20 },
    /* 20 */ { "T",   112, 20, 20, 17, MO5Key::T,       5, 36,  19,  21 },
    /* 21 */ { "Y",   132, 20, 20, 17, MO5Key::Y,       6, 37,  20,  22 },
    /* 22 */ { "U",   152, 20, 20, 17, MO5Key::U,       7, 38,  21,  23 },
    /* 23 */ { "I",   172, 20, 20, 17, MO5Key::I,       8, 39,  22,  24 },
    /* 24 */ { "O",   192, 20, 20, 17, MO5Key::O,       9, 40,  23,  25 },
    /* 25 */ { "P",   212, 20, 20, 17, MO5Key::P,      10, 41,  24,  26 },
    /* 26 */ { "/",   232, 20, 20, 17, MO5Key::DIV,    11, 42,  25,  27 },
    /* 27 */ { "*",   252, 20, 20, 17, MO5Key::STAR,   12, 42,  26,  28 },
    /* 28 */ { "<-",  272, 20, 20, 17, MO5Key::LEFT,   13, 43,  27,  29 },
    /* 29 */ { "->",  292, 20, 20, 17, MO5Key::RIGHT,  14, 43,  28,  -1 },

    // Row 2 (14 keys): y=38, h=17
    /* 30 */ { "RAZ",   2, 38, 20, 17, MO5Key::RAZ,    15, 44,  -1,  31 },
    /* 31 */ { "<",    22, 38, 20, 17, MO5Key::BACKSPACE, 16, 45,  30,  32 },
    /* 32 */ { "Q",    42, 38, 20, 17, MO5Key::Q,      17, 46,  31,  33 },
    /* 33 */ { "S",    62, 38, 20, 17, MO5Key::S,      18, 47,  32,  34 },
    /* 34 */ { "D",    82, 38, 20, 17, MO5Key::D,      19, 48,  33,  35 },
    /* 35 */ { "F",   102, 38, 20, 17, MO5Key::F,      20, 49,  34,  36 },
    /* 36 */ { "G",   122, 38, 20, 17, MO5Key::G,      21, 50,  35,  37 },
    /* 37 */ { "H",   142, 38, 20, 17, MO5Key::H,      22, 51,  36,  38 },
    /* 38 */ { "J",   162, 38, 20, 17, MO5Key::J,      23, 52,  37,  39 },
    /* 39 */ { "K",   182, 38, 20, 17, MO5Key::K,      24, 53,  38,  40 },
    /* 40 */ { "L",   202, 38, 20, 17, MO5Key::L,      25, 54,  39,  41 },
    /* 41 */ { "M",   222, 38, 20, 17, MO5Key::M,      26, 54,  40,  42 },
    /* 42 */ { "ENT", 242, 38, 30, 17, MO5Key::ENTER,  27, 55,  41,  43 },
    /* 43 */ { "v",   282, 38, 20, 17, MO5Key::DOWN,   29, 56,  42,  -1 },

    // Row 3 (13 keys): y=56, h=17 — SHIFT is 40px wide, then 10px gap
    /* 44 */ { "",      2, 56, 40, 17, MO5Key::SHIFT,  30, 57,  -1,  45 },
    /* 45 */ { "W",    52, 56, 20, 17, MO5Key::W,      31, 57,  44,  46 },
    /* 46 */ { "X",    72, 56, 20, 17, MO5Key::X,      32, 57,  45,  47 },
    /* 47 */ { "C",    92, 56, 20, 17, MO5Key::C,      33, 57,  46,  48 },
    /* 48 */ { "V",   112, 56, 20, 17, MO5Key::V,      34, 57,  47,  49 },
    /* 49 */ { "B",   132, 56, 20, 17, MO5Key::B,      35, 57,  48,  50 },
    /* 50 */ { "N",   152, 56, 20, 17, MO5Key::N,      36, 57,  49,  51 },
    /* 51 */ { ",",   172, 56, 20, 17, MO5Key::COMMA,  37, 57,  50,  52 },
    /* 52 */ { ".",   192, 56, 20, 17, MO5Key::PERIOD,  38, 57,  51,  53 },
    /* 53 */ { "@",   212, 56, 20, 17, MO5Key::AT,     39, 57,  52,  54 },
    /* 54 */ { "BAS", 232, 56, 40, 17, MO5Key::BASIC,  40, 57,  53,  55 },
    /* 55 */ { "INS", 272, 56, 20, 17, MO5Key::INS,    42, -1,  54,  56 },
    /* 56 */ { "EFF", 292, 56, 20, 17, MO5Key::EFF,    43, -1,  55,  -1 },

    // Row 4 (1 key): y=74, h=17
    /* 57 */ { "SPC", 102, 74,  80, 17, MO5Key::SPACE, 47, -1,  -1,  -1 },
};

// --- Stub implementations (to be filled in tasks 2.2, 2.3, 2.4, 4.1, 4.2, 4.3) ---

VirtualKeyboard::VirtualKeyboard() = default;

void VirtualKeyboard::toggle_visible() {
    visible_ = !visible_;
}

bool VirtualKeyboard::is_visible() const {
    return visible_;
}

void VirtualKeyboard::move_cursor(Direction dir) {
    int next = -1;
    switch (dir) {
        case Direction::Up:    next = LAYOUT[cursor_index_].nav_up;    break;
        case Direction::Down:  next = LAYOUT[cursor_index_].nav_down;  break;
        case Direction::Left:  next = LAYOUT[cursor_index_].nav_left;  break;
        case Direction::Right: next = LAYOUT[cursor_index_].nav_right; break;
    }
    if (next >= 0 && next < KEY_COUNT) {
        cursor_index_ = next;
    }
}

MO5Key VirtualKeyboard::press_selected() const {
    int idx = std::clamp(cursor_index_, 0, KEY_COUNT - 1);
    return LAYOUT[idx].mo5_key;
}

int VirtualKeyboard::get_cursor_index() const {
    return cursor_index_;
}

int VirtualKeyboard::hit_test(int x, int y, int fb_height) const {
    int y_off = get_y_offset(fb_height);
    int x_off = get_x_offset(320);  // framebuffer width is always 320
    int adj_y = y - y_off;
    int adj_x = x - x_off;
    for (int i = 0; i < KEY_COUNT; ++i) {
        const auto& k = LAYOUT[i];
        if (adj_x >= k.x && adj_x < k.x + k.width &&
            adj_y >= k.y && adj_y < k.y + k.height) {
            return i;
        }
    }
    return -1;
}

MO5Key VirtualKeyboard::get_key_at(int index) const {
    if (index < 0 || index >= KEY_COUNT) return MO5Key::SPACE;
    return LAYOUT[index].mo5_key;
}

void VirtualKeyboard::set_cursor_index(int index) {
    if (index >= 0 && index < KEY_COUNT)
        cursor_index_ = index;
}

void VirtualKeyboard::toggle_shift() {
    shift_active_ = !shift_active_;
}

bool VirtualKeyboard::is_shift_active() const {
    return shift_active_;
}

void VirtualKeyboard::toggle_basic() {
    basic_active_ = !basic_active_;
}

bool VirtualKeyboard::is_basic_active() const {
    return basic_active_;
}

void VirtualKeyboard::toggle_acc() {
    acc_active_ = !acc_active_;
}

bool VirtualKeyboard::is_acc_active() const {
    return acc_active_;
}

void VirtualKeyboard::toggle_cnt() {
    cnt_active_ = !cnt_active_;
}

bool VirtualKeyboard::is_cnt_active() const {
    return cnt_active_;
}

MO5Key VirtualKeyboard::active_modifier() const {
    if (shift_active_) return MO5Key::SHIFT;
    if (basic_active_) return MO5Key::BASIC;
    if (acc_active_) return MO5Key::ACC;
    if (cnt_active_) return MO5Key::CNT;
    return MO5Key::SPACE;  // sentinel: no modifier
}

void VirtualKeyboard::toggle_position() {
    position_ = (position_ == VKBPosition::Bottom) ? VKBPosition::Top : VKBPosition::Bottom;
}

void VirtualKeyboard::set_position(VKBPosition pos) {
    position_ = pos;
}

VKBPosition VirtualKeyboard::get_position() const {
    return position_;
}

void VirtualKeyboard::set_transparency(VKBTransparency t) {
    transparency_ = t;
}

VKBTransparency VirtualKeyboard::get_transparency() const {
    return transparency_;
}

void VirtualKeyboard::render(uint32_t* framebuffer, int fb_width, int fb_height) const {
    if (!visible_ || framebuffer == nullptr) return;

    int y_offset = get_y_offset(fb_height);
    int x_offset = get_x_offset(fb_width);

    // Compute alpha from transparency level
    uint8_t alpha;
    switch (transparency_) {
        case VKBTransparency::Opaque:          alpha = 255; break;
        case VKBTransparency::SemiTransparent: alpha = 160; break;
        case VKBTransparency::Transparent:     alpha = 80;  break;
        default:                               alpha = 255; break;
    }

    // Draw panel background
    draw_rect(framebuffer, fb_width, fb_height,
              x_offset, y_offset, VKB_WIDTH, VKB_HEIGHT,
              vkb_colors::PANEL, alpha);

    // Draw each key
    int idx = std::clamp(cursor_index_, 0, KEY_COUNT - 1);
    for (int i = 0; i < KEY_COUNT; ++i) {
        const auto& key = LAYOUT[i];

        // Determine face color and text color
        uint32_t face_color;
        uint32_t text_color;
        bool draw_text = true;

        if (i == idx) {
            // Cursor is on this key
            face_color = vkb_colors::CURSOR;
            text_color = vkb_colors::CURSOR_TEXT;
        } else if (key.mo5_key == MO5Key::SHIFT && shift_active_) {
            face_color = vkb_colors::CURSOR;
            text_color = vkb_colors::CURSOR_TEXT;
        } else if (key.mo5_key == MO5Key::SHIFT) {
            face_color = vkb_colors::KEY_SHIFT;
            draw_text = false;
        } else if (key.mo5_key == MO5Key::BASIC && basic_active_) {
            face_color = vkb_colors::CURSOR;
            text_color = vkb_colors::CURSOR_TEXT;
        } else if (key.mo5_key == MO5Key::BASIC) {
            face_color = vkb_colors::KEY_BASIC;
            text_color = vkb_colors::TEXT;
        } else if (i == 13 && acc_active_) {  // ACC key at index 13
            face_color = vkb_colors::CURSOR;
            text_color = vkb_colors::CURSOR_TEXT;
        } else if (key.mo5_key == MO5Key::CNT && cnt_active_) {
            face_color = vkb_colors::CURSOR;
            text_color = vkb_colors::CURSOR_TEXT;
        } else {
            face_color = vkb_colors::KEY_FACE;
            text_color = vkb_colors::TEXT;
        }

        // Draw key border (full key rect)
        draw_rect(framebuffer, fb_width, fb_height,
                  key.x + x_offset, key.y + y_offset, key.width, key.height,
                  vkb_colors::BORDER, alpha);

        // Draw key face (inset by 1px)
        draw_rect(framebuffer, fb_width, fb_height,
                  key.x + x_offset + 1, key.y + y_offset + 1, key.width - 2, key.height - 2,
                  face_color, alpha);

        // Draw label (centered)
        if (draw_text && key.label && key.label[0] != '\0') {
            int label_len = static_cast<int>(std::strlen(key.label));
            int text_pixel_width = label_len * 6 - 1;
            int text_x = key.x + x_offset + (key.width - text_pixel_width) / 2;
            int text_y = key.y + y_offset + (key.height - FONT_CHAR_HEIGHT) / 2;
            draw_label(framebuffer, fb_width, fb_height,
                       text_x, text_y, key.label, text_color, alpha);
        }
    }
}

int VirtualKeyboard::get_y_offset(int fb_height) const {
    if (position_ == VKBPosition::Top) return 0;
    return fb_height - VKB_HEIGHT;
}

int VirtualKeyboard::get_x_offset(int fb_width) const {
    return (fb_width - VKB_WIDTH) / 2;
}

uint32_t VirtualKeyboard::blend_pixel(uint32_t bg, uint32_t fg, uint8_t alpha) const {
    if (alpha == 255) return fg;
    if (alpha == 0) return bg;
    uint8_t inv = 255 - alpha;
    uint8_t r = static_cast<uint8_t>(((fg >> 16 & 0xFF) * alpha + (bg >> 16 & 0xFF) * inv) / 255);
    uint8_t g = static_cast<uint8_t>(((fg >>  8 & 0xFF) * alpha + (bg >>  8 & 0xFF) * inv) / 255);
    uint8_t b = static_cast<uint8_t>(((fg       & 0xFF) * alpha + (bg       & 0xFF) * inv) / 255);
    return (r << 16) | (g << 8) | b;
}

void VirtualKeyboard::draw_rect(uint32_t* fb, int fb_w, int fb_h,
                                int x, int y, int w, int h,
                                uint32_t color, uint8_t alpha) const {
    int x0 = std::max(x, 0);
    int y0 = std::max(y, 0);
    int x1 = std::min(x + w, fb_w);
    int y1 = std::min(y + h, fb_h);
    for (int py = y0; py < y1; ++py) {
        for (int px = x0; px < x1; ++px) {
            fb[py * fb_w + px] = blend_pixel(fb[py * fb_w + px], color, alpha);
        }
    }
}

void VirtualKeyboard::draw_char(uint32_t* fb, int fb_w, int fb_h,
                                int x, int y, char ch,
                                uint32_t color, uint8_t alpha) const {
    if (ch < 32 || ch > 127) return;
    const uint8_t* glyph = FONT_DATA[ch - 32];
    for (int row = 0; row < FONT_CHAR_HEIGHT; ++row) {
        int py = y + row;
        if (py < 0 || py >= fb_h) continue;
        uint8_t bits = glyph[row];
        for (int col = 0; col < FONT_CHAR_WIDTH; ++col) {
            if (!(bits & (0x80 >> col))) continue;
            int px = x + col;
            if (px < 0 || px >= fb_w) continue;
            int idx = py * fb_w + px;
            fb[idx] = blend_pixel(fb[idx], color, alpha);
        }
    }
}

void VirtualKeyboard::draw_label(uint32_t* fb, int fb_w, int fb_h,
                                 int x, int y, const char* text,
                                 uint32_t color, uint8_t alpha) const {
    if (!text) return;
    int cx = x;
    for (const char* p = text; *p; ++p) {
        draw_char(fb, fb_w, fb_h, cx, y, *p, color, alpha);
        cx += FONT_CHAR_WIDTH + 1;  // 6px stride
    }
}

} // namespace crayon
