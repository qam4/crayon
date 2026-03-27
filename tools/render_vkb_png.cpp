#include "vkeyboard.h"
#include <cstdio>
#include <cstring>
#include <vector>

using namespace crayon;

// Minimal PPM writer — we'll convert to PNG externally
int main() {
    constexpr int W = VirtualKeyboard::VKB_WIDTH;
    constexpr int H = VirtualKeyboard::VKB_HEIGHT;
    constexpr int SCALE = 3; // 3x upscale for readability
    constexpr int OUT_W = W * SCALE;
    constexpr int OUT_H = H * SCALE;

    // Render VKB into a framebuffer at native resolution
    // Use fb_height = VKB_HEIGHT so position=Bottom puts it at y=0
    std::vector<uint32_t> fb(W * H, 0x00000000);

    VirtualKeyboard vkb;
    vkb.toggle_visible();
    vkb.set_position(VKBPosition::Top);
    vkb.set_transparency(VKBTransparency::Opaque);
    vkb.render(fb.data(), W, H);

    // Upscale to OUT_W x OUT_H
    std::vector<uint32_t> out(OUT_W * OUT_H);
    for (int y = 0; y < OUT_H; ++y) {
        for (int x = 0; x < OUT_W; ++x) {
            out[y * OUT_W + x] = fb[(y / SCALE) * W + (x / SCALE)];
        }
    }

    // Write PPM (P6 binary)
    FILE* f = std::fopen("doc/vkb_layout.ppm", "wb");
    if (!f) { std::fprintf(stderr, "Cannot open output file\n"); return 1; }
    std::fprintf(f, "P6\n%d %d\n255\n", OUT_W, OUT_H);
    for (int i = 0; i < OUT_W * OUT_H; ++i) {
        uint32_t px = out[i];
        uint8_t rgb[3] = {
            static_cast<uint8_t>((px >> 16) & 0xFF),
            static_cast<uint8_t>((px >>  8) & 0xFF),
            static_cast<uint8_t>( px        & 0xFF)
        };
        std::fwrite(rgb, 1, 3, f);
    }
    std::fclose(f);
    std::printf("Wrote doc/vkb_layout.ppm (%dx%d, %dx upscale)\n", OUT_W, OUT_H, SCALE);
    return 0;
}
