#include <gtest/gtest.h>
#include "vkeyboard.h"

using namespace crayon;

// Unit tests for VirtualKeyboard

TEST(VKBUnit, DefaultState) {
    VirtualKeyboard vkb;
    EXPECT_FALSE(vkb.is_visible());
    EXPECT_EQ(vkb.get_position(), VKBPosition::Bottom);
    EXPECT_EQ(vkb.get_transparency(), VKBTransparency::Opaque);
    EXPECT_EQ(vkb.get_cursor_index(), 0);
    EXPECT_FALSE(vkb.is_shift_active());
}

TEST(VKBUnit, StopAtIndex0) {
    EXPECT_EQ(VirtualKeyboard::LAYOUT[0].mo5_key, MO5Key::STOP);
    EXPECT_STREQ(VirtualKeyboard::LAYOUT[0].label, "STP");
}

TEST(VKBUnit, SpaceAtIndex57) {
    EXPECT_EQ(VirtualKeyboard::LAYOUT[57].mo5_key, MO5Key::SPACE);
    EXPECT_STREQ(VirtualKeyboard::LAYOUT[57].label, "SPC");
    EXPECT_EQ(VirtualKeyboard::LAYOUT[57].width, 80);
}

TEST(VKBUnit, EnterKeyWider) {
    // ENTER at index 42 should be wider than standard 20px
    EXPECT_GT(VirtualKeyboard::LAYOUT[42].width, 20);
}

TEST(VKBUnit, BasicKeyWider) {
    // BASIC at index 54 should be wider than standard 20px
    EXPECT_GT(VirtualKeyboard::LAYOUT[54].width, 20);
    EXPECT_EQ(VirtualKeyboard::LAYOUT[54].width, 40);
}

TEST(VKBUnit, BlendPixelEdgeCases) {
    VirtualKeyboard vkb;
    // We can't call private blend_pixel directly, but we can test via render behavior
    // Test that render with hidden VKB doesn't modify framebuffer
    uint32_t fb[320 * 200] = {};
    fb[0] = 0x00FF0000;  // Red pixel
    vkb.render(fb, 320, 200);  // VKB is hidden by default
    EXPECT_EQ(fb[0], 0x00FF0000u);  // Should be unchanged
}

TEST(VKBUnit, ShiftToggle) {
    VirtualKeyboard vkb;
    EXPECT_FALSE(vkb.is_shift_active());
    vkb.toggle_shift();
    EXPECT_TRUE(vkb.is_shift_active());
    vkb.toggle_shift();
    EXPECT_FALSE(vkb.is_shift_active());
}
