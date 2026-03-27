#include "vkeyboard.h"
#include <cstdio>
#include <cstring>

using namespace crayon;

int main() {
    // Render a simple ASCII bitmap of the VKB layout
    // Scale: 1 char = 4 pixels wide, 1 char = 4 pixels tall
    constexpr int SCALE_X = 4;
    constexpr int SCALE_Y = 4;
    constexpr int W = (VirtualKeyboard::VKB_WIDTH + SCALE_X - 1) / SCALE_X + 1;
    constexpr int H = (VirtualKeyboard::VKB_HEIGHT + SCALE_Y - 1) / SCALE_Y + 1;

    char grid[H][W + 1];
    std::memset(grid, '.', sizeof(grid));
    for (int r = 0; r < H; ++r) grid[r][W] = '\0';

    for (int i = 0; i < VirtualKeyboard::KEY_COUNT; ++i) {
        const auto& k = VirtualKeyboard::LAYOUT[i];
        int x0 = k.x / SCALE_X;
        int y0 = k.y / SCALE_Y;
        int x1 = (k.x + k.width) / SCALE_X;
        int y1 = (k.y + k.height) / SCALE_Y;

        // Draw border
        for (int x = x0; x <= x1 && x < W; ++x) {
            if (y0 < H) grid[y0][x] = '-';
            if (y1 < H) grid[y1][x] = '-';
        }
        for (int y = y0; y <= y1 && y < H; ++y) {
            if (x0 < W) grid[y][x0] = '|';
            if (x1 < W) grid[y][x1] = '|';
        }

        // Place label centered
        int label_len = k.label ? static_cast<int>(std::strlen(k.label)) : 0;
        int mid_y = (y0 + y1) / 2;
        int mid_x = (x0 + x1) / 2 - label_len / 2;
        if (mid_y >= 0 && mid_y < H) {
            for (int c = 0; c < label_len && (mid_x + c) < W; ++c) {
                if (mid_x + c >= 0)
                    grid[mid_y][mid_x + c] = k.label[c];
            }
        }
    }

    std::printf("VKB Layout (%d x %d px, scale 1:%d)\n\n", 
                VirtualKeyboard::VKB_WIDTH, VirtualKeyboard::VKB_HEIGHT, SCALE_X);
    for (int r = 0; r < H; ++r) {
        std::printf("%s\n", grid[r]);
    }
    std::printf("\n");

    // Also print a table
    std::printf("Index | Label | x   | y  | w  | h  | scancode\n");
    std::printf("------|-------|-----|----|----|----|---------\n");
    for (int i = 0; i < VirtualKeyboard::KEY_COUNT; ++i) {
        const auto& k = VirtualKeyboard::LAYOUT[i];
        std::printf("  %2d  | %-5s | %3d | %2d | %2d | %2d | 0x%02X\n",
                    i, k.label[0] ? k.label : "(SHF)",
                    k.x, k.y, k.width, k.height,
                    static_cast<unsigned>(k.mo5_key));
    }
    return 0;
}
