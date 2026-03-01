// Boot diagnostic: verify BASIC prompt renders correctly
#include "../include/emulator_core.h"
#include <iostream>
#include <iomanip>
#include <cstring>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "../third_party/stb_image_write.h"

int main() {
    crayon::Configuration config;
    crayon::EmulatorCore emu(config);
    emu.load_basic_rom("roms/basic5.rom");
    emu.load_monitor_rom("roms/mo5.rom");
    emu.reset();
    auto& mem = emu.get_memory();

    // Run 300 frames
    for (int f = 0; f < 300; ++f)
        emu.run_frame();

    auto ms = mem.get_state();
    const uint32_t* fb = emu.get_framebuffer();

    // Render ASCII art of top 25 rows from framebuffer
    // Cyan pixel = background (space), Blue pixel = foreground (#)
    std::cout << "Framebuffer top-left (25 rows x 80 cols, 4px per char):\n";
    for (int y = 0; y < 25; ++y) {
        for (int cx = 0; cx < 80; ++cx) {
            int x = cx * 4;
            // Sample center of 4-pixel block
            uint32_t p = fb[y * 320 + x + 2];
            // Blue=0x0000FFFF=foreground, Cyan=0x00FFFFFF=background
            bool is_fg = (p != 0x00FFFFFF);
            std::cout << (is_fg ? '#' : ' ');
        }
        std::cout << "\n";
    }

    // Save screenshot
    std::vector<uint8_t> pixels(320 * 200 * 4);
    for (int i = 0; i < 320 * 200; ++i) {
        uint32_t c = fb[i];
        pixels[i * 4 + 0] = (c >> 24) & 0xFF;
        pixels[i * 4 + 1] = (c >> 16) & 0xFF;
        pixels[i * 4 + 2] = (c >> 8) & 0xFF;
        pixels[i * 4 + 3] = c & 0xFF;
    }
    stbi_write_png("screenshots/boot_diag.png", 320, 200, 4, pixels.data(), 320 * 4);
    std::cout << "\nScreenshot: screenshots/boot_diag.png\n";

    return 0;
}
