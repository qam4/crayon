#include "vkeyboard.h"
#include <algorithm>
#include <cstring>

namespace crayon {

// MO5 AZERTY keyboard layout — 58 keys across 5 rows + 2 extra keys (INS, RAZ)
// Row 0: STOP 1 2 3 4 5 6 7 8 9 0 + ACC ACC2 EFF  (15 keys)
// Row 1: CNT  A Z E R T Y U I O P *  ENT           (13 keys)
// Row 2: SHIFT Q S D F G H J K L M @               (12 keys)
// Row 3: BASIC W X C V B N , . / <- -> ^ v INS RAZ (16 keys)
// Row 4: SPACE                                       (1 key, wide)
// Total: 15 + 13 + 12 + 16 + 1 = 57 entries but INS and RAZ bring us to 58 unique MO5Keys

const VKBKey VirtualKeyboard::LAYOUT[] = {
    // Row 0: STOP 1 2 3 4 5 6 7 8 9 0 + ACC ACC2 EFF
    { MO5Key::STOP,  "STOP", 0,  0, 1 },
    { MO5Key::Key1,  "1",    0,  1, 1 },
    { MO5Key::Key2,  "2",    0,  2, 1 },
    { MO5Key::Key3,  "3",    0,  3, 1 },
    { MO5Key::Key4,  "4",    0,  4, 1 },
    { MO5Key::Key5,  "5",    0,  5, 1 },
    { MO5Key::Key6,  "6",    0,  6, 1 },
    { MO5Key::Key7,  "7",    0,  7, 1 },
    { MO5Key::Key8,  "8",    0,  8, 1 },
    { MO5Key::Key9,  "9",    0,  9, 1 },
    { MO5Key::Key0,  "0",    0, 10, 1 },
    { MO5Key::PLUS,  "+",    0, 11, 1 },
    { MO5Key::ACC,   "ACC",  0, 12, 1 },
    { MO5Key::ACC2,  "ACC2", 0, 13, 1 },
    { MO5Key::EFF,   "EFF",  0, 14, 1 },

    // Row 1: CNT A Z E R T Y U I O P * ENT
    { MO5Key::CNT,   "CNT",  1,  0, 1 },
    { MO5Key::A,     "A",    1,  1, 1 },
    { MO5Key::Z,     "Z",    1,  2, 1 },
    { MO5Key::E,     "E",    1,  3, 1 },
    { MO5Key::R,     "R",    1,  4, 1 },
    { MO5Key::T,     "T",    1,  5, 1 },
    { MO5Key::Y,     "Y",    1,  6, 1 },
    { MO5Key::U,     "U",    1,  7, 1 },
    { MO5Key::I,     "I",    1,  8, 1 },
    { MO5Key::O,     "O",    1,  9, 1 },
    { MO5Key::P,     "P",    1, 10, 1 },
    { MO5Key::STAR,  "*",    1, 11, 1 },
    { MO5Key::ENTER, "ENT",  1, 12, 1 },

    // Row 2: SHIFT Q S D F G H J K L M @
    { MO5Key::SHIFT, "SHF",  2,  0, 1 },
    { MO5Key::Q,     "Q",    2,  1, 1 },
    { MO5Key::S,     "S",    2,  2, 1 },
    { MO5Key::D,     "D",    2,  3, 1 },
    { MO5Key::F,     "F",    2,  4, 1 },
    { MO5Key::G,     "G",    2,  5, 1 },
    { MO5Key::H,     "H",    2,  6, 1 },
    { MO5Key::J,     "J",    2,  7, 1 },
    { MO5Key::K,     "K",    2,  8, 1 },
    { MO5Key::L,     "L",    2,  9, 1 },
    { MO5Key::SLASH, "M",    2, 10, 1 },
    { MO5Key::AT,    "@",    2, 11, 1 },

    // Row 3: BASIC W X C V B N , . / <- -> ^ v INS RAZ
    { MO5Key::BASIC, "BAS",  3,  0, 1 },
    { MO5Key::W,     "W",    3,  1, 1 },
    { MO5Key::X,     "X",    3,  2, 1 },
    { MO5Key::C,     "C",    3,  3, 1 },
    { MO5Key::V,     "V",    3,  4, 1 },
    { MO5Key::B,     "B",    3,  5, 1 },
    { MO5Key::N,     "N",    3,  6, 1 },
    { MO5Key::M,     ",",    3,  7, 1 },
    { MO5Key::COMMA, ".",    3,  8, 1 },
    { MO5Key::DOT,   "/",    3,  9, 1 },
    { MO5Key::LEFT,  "<-",   3, 10, 1 },
    { MO5Key::RIGHT, "->",   3, 11, 1 },
    { MO5Key::UP,    "^",    3, 12, 1 },
    { MO5Key::DOWN,  "v",    3, 13, 1 },
    { MO5Key::INS,   "INS",  3, 14, 1 },
    { MO5Key::RAZ,   "RAZ",  3, 15, 1 },

    // Row 4: SPACE (wide key)
    { MO5Key::SPACE, "SPACE", 4, 0, 6 },

    // MINUS key (not in main rows above, add to row 0 area — but we already have 15 keys there)
    // Actually MINUS is the - key on MO5, mapped to Key0 area. Let's check: we have 57 entries above.
    // The 58th key is MINUS (0x26). Add it to row 0 after EFF.
    { MO5Key::MINUS, "-",    0, 15, 1 },
};

// Row sizes: row0=16, row1=13, row2=12, row3=16, row4=1
const int VirtualKeyboard::ROW_COUNT = 5;
const int VirtualKeyboard::MAX_COL_COUNT = 16;

VirtualKeyboard::VirtualKeyboard() = default;

void VirtualKeyboard::toggle_visible() {
    visible_ = !visible_;
}

bool VirtualKeyboard::is_visible() const {
    return visible_;
}

void VirtualKeyboard::toggle_shift() {
    shift_active_ = !shift_active_;
}

void VirtualKeyboard::toggle_position() {
    position_ = (position_ == VKBPosition::Bottom) ? VKBPosition::Top : VKBPosition::Bottom;
}

void VirtualKeyboard::set_position(VKBPosition pos) {
    position_ = pos;
}

void VirtualKeyboard::set_transparency(VKBTransparency t) {
    transparency_ = t;
}

VKBPosition VirtualKeyboard::get_position() const {
    return position_;
}

bool VirtualKeyboard::is_shift_active() const {
    return shift_active_;
}

// Helper: get the number of columns in a given row
static int row_col_count(int row) {
    // Count keys in each row from the LAYOUT
    static const int counts[] = { 16, 13, 12, 16, 1 };
    if (row < 0 || row >= VirtualKeyboard::ROW_COUNT) return 0;
    return counts[row];
}

int VirtualKeyboard::get_key_index(int row, int col) const {
    constexpr int total = sizeof(LAYOUT) / sizeof(LAYOUT[0]);
    for (int i = 0; i < total; ++i) {
        if (LAYOUT[i].row == row && LAYOUT[i].col == col)
            return i;
    }
    return -1;
}

void VirtualKeyboard::clamp_cursor() {
    if (cursor_row_ < 0) cursor_row_ = 0;
    if (cursor_row_ >= ROW_COUNT) cursor_row_ = ROW_COUNT - 1;

    int cols = row_col_count(cursor_row_);
    if (cursor_col_ < 0) cursor_col_ = 0;
    if (cursor_col_ >= cols) cursor_col_ = cols - 1;
}

void VirtualKeyboard::move_cursor(int dx, int dy) {
    cursor_col_ += dx;
    cursor_row_ += dy;
    clamp_cursor();
}

MO5Key VirtualKeyboard::get_selected_key() const {
    int idx = get_key_index(cursor_row_, cursor_col_);
    if (idx >= 0) return LAYOUT[idx].mo5_key;
    // Fallback: return first key in current row
    constexpr int total = sizeof(LAYOUT) / sizeof(LAYOUT[0]);
    for (int i = 0; i < total; ++i) {
        if (LAYOUT[i].row == cursor_row_) return LAYOUT[i].mo5_key;
    }
    return MO5Key::SPACE;
}

MO5Key VirtualKeyboard::press_selected() {
    return get_selected_key();
}

uint32_t VirtualKeyboard::blend_pixel(uint32_t bg, uint32_t fg, uint8_t alpha) const {
    if (alpha == 255) return fg;
    if (alpha == 0) return bg;

    uint32_t bg_r = (bg >> 16) & 0xFF;
    uint32_t bg_g = (bg >>  8) & 0xFF;
    uint32_t bg_b =  bg        & 0xFF;

    uint32_t fg_r = (fg >> 16) & 0xFF;
    uint32_t fg_g = (fg >>  8) & 0xFF;
    uint32_t fg_b =  fg        & 0xFF;

    uint32_t inv = 255 - alpha;
    uint32_t r = (fg_r * alpha + bg_r * inv) / 255;
    uint32_t g = (fg_g * alpha + bg_g * inv) / 255;
    uint32_t b = (fg_b * alpha + bg_b * inv) / 255;

    return (r << 16) | (g << 8) | b;
}

void VirtualKeyboard::render(uint32_t* framebuffer, int fb_width, int fb_height) {
    if (!visible_ || !framebuffer) return;

    uint8_t alpha;
    switch (transparency_) {
        case VKBTransparency::Opaque:          alpha = 255; break;
        case VKBTransparency::SemiTransparent: alpha = 160; break;
        case VKBTransparency::Transparent:     alpha = 80;  break;
        default:                               alpha = 255; break;
    }

    const int vkb_height = 50;  // 5 rows x 10px each
    const int row_height = 10;
    const int key_width = fb_width / MAX_COL_COUNT;

    int y_offset;
    if (position_ == VKBPosition::Top) {
        y_offset = 0;
    } else {
        y_offset = fb_height - vkb_height;
    }

    const uint32_t key_bg    = 0x00404040;  // Dark gray
    const uint32_t key_hi    = 0x004080FF;  // Bright blue (highlighted)
    const uint32_t text_col  = 0x00FFFFFF;  // White text
    const uint32_t border_col = 0x00202020; // Dark border

    constexpr int total_keys = sizeof(LAYOUT) / sizeof(LAYOUT[0]);

    for (int i = 0; i < total_keys; ++i) {
        const VKBKey& key = LAYOUT[i];

        int kx = key.col * key_width;
        int ky = y_offset + key.row * row_height;
        int kw = key.width * key_width;
        int kh = row_height;

        bool highlighted = (key.row == cursor_row_ && key.col == cursor_col_);
        uint32_t bg_color = highlighted ? key_hi : key_bg;

        // Draw key background with 1px border
        for (int py = ky; py < ky + kh && py < fb_height && py >= 0; ++py) {
            for (int px = kx; px < kx + kw && px < fb_width; ++px) {
                bool is_border = (py == ky || py == ky + kh - 1 || px == kx || px == kx + kw - 1);
                uint32_t color = is_border ? border_col : bg_color;
                int idx = py * fb_width + px;
                framebuffer[idx] = blend_pixel(framebuffer[idx], color, alpha);
            }
        }

        // Draw label — simple block characters (each char is ~3x5 pixels)
        const char* label = key.label;
        int label_len = static_cast<int>(std::strlen(label));
        int char_w = 4;  // pixels per character width
        int char_h = 5;  // pixels per character height
        int text_x = kx + (kw - label_len * char_w) / 2;
        int text_y = ky + (kh - char_h) / 2;

        // Draw each character as a small filled block (enough to identify keys)
        for (int c = 0; c < label_len; ++c) {
            int cx = text_x + c * char_w;
            // Draw a simple 3x5 block for each character
            for (int py = text_y + 1; py < text_y + char_h && py < fb_height && py >= 0; ++py) {
                for (int px = cx; px < cx + 3 && px < fb_width && px >= 0; ++px) {
                    int idx = py * fb_width + px;
                    framebuffer[idx] = blend_pixel(framebuffer[idx], text_col, alpha);
                }
            }
        }
    }
}

} // namespace crayon
