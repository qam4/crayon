#include <gtest/gtest.h>
#include "input_handler.h"
#include "memory_system.h"
#include "audio_system.h"

using namespace crayon;

// --- InputHandler joystick unit tests ---

TEST(JoystickUnit, DefaultStateAllReleased) {
    InputHandler ih;
    EXPECT_EQ(ih.get_joystick_port_a(), 0xFF);
    EXPECT_EQ(ih.get_joystick_port_b_fire(), 0xC0);
}

TEST(JoystickUnit, SetDirectionPort0) {
    InputHandler ih;
    ih.set_joystick_direction(0, true, false, true, false);  // up + left
    // Port A: bit 0 (up) cleared, bit 2 (left) cleared → 0xFF & ~0x01 & ~0x04 = 0xFA
    EXPECT_EQ(ih.get_joystick_port_a(), 0xFA);
}

TEST(JoystickUnit, SetDirectionPort1) {
    InputHandler ih;
    ih.set_joystick_direction(1, false, true, false, true);  // down + right
    // Port A: bit 5 (joy2 down) cleared, bit 7 (joy2 right) cleared → 0xFF & ~0x20 & ~0x80 = 0x5F
    EXPECT_EQ(ih.get_joystick_port_a(), 0x5F);
}

TEST(JoystickUnit, FirePort0) {
    InputHandler ih;
    ih.set_joystick_fire(0, true);
    // Port B: bit 6 cleared → 0xC0 & ~0x40 = 0x80
    EXPECT_EQ(ih.get_joystick_port_b_fire(), 0x80);
}

TEST(JoystickUnit, FirePort1) {
    InputHandler ih;
    ih.set_joystick_fire(1, true);
    // Port B: bit 7 cleared → 0xC0 & ~0x80 = 0x40
    EXPECT_EQ(ih.get_joystick_port_b_fire(), 0x40);
}

TEST(JoystickUnit, BothFiresPressed) {
    InputHandler ih;
    ih.set_joystick_fire(0, true);
    ih.set_joystick_fire(1, true);
    EXPECT_EQ(ih.get_joystick_port_b_fire(), 0x00);
}

TEST(JoystickUnit, ResetClearsJoystick) {
    InputHandler ih;
    ih.set_joystick_direction(0, true, true, true, true);
    ih.set_joystick_fire(0, true);
    ih.set_joystick_direction(1, true, true, true, true);
    ih.set_joystick_fire(1, true);
    ih.reset();
    EXPECT_EQ(ih.get_joystick_port_a(), 0xFF);
    EXPECT_EQ(ih.get_joystick_port_b_fire(), 0xC0);
}

TEST(JoystickUnit, OutOfRangePortIgnored) {
    InputHandler ih;
    ih.set_joystick_direction(2, true, true, true, true);
    ih.set_joystick_direction(-1, true, true, true, true);
    ih.set_joystick_fire(2, true);
    ih.set_joystick_fire(-1, true);
    EXPECT_EQ(ih.get_joystick_port_a(), 0xFF);
    EXPECT_EQ(ih.get_joystick_port_b_fire(), 0xC0);
}

TEST(JoystickUnit, StateRoundTrip) {
    InputHandler ih;
    ih.set_joystick_direction(0, true, false, true, false);
    ih.set_joystick_fire(0, true);
    ih.set_joystick_direction(1, false, true, false, true);
    ih.set_joystick_fire(1, false);

    InputState state = ih.get_state();
    EXPECT_TRUE(state.joy[0].up);
    EXPECT_FALSE(state.joy[0].down);
    EXPECT_TRUE(state.joy[0].left);
    EXPECT_FALSE(state.joy[0].right);
    EXPECT_TRUE(state.joy[0].fire);
    EXPECT_FALSE(state.joy[1].up);
    EXPECT_TRUE(state.joy[1].down);
    EXPECT_FALSE(state.joy[1].left);
    EXPECT_TRUE(state.joy[1].right);
    EXPECT_FALSE(state.joy[1].fire);

    InputHandler ih2;
    ih2.set_state(state);
    EXPECT_EQ(ih2.get_joystick_port_a(), ih.get_joystick_port_a());
    EXPECT_EQ(ih2.get_joystick_port_b_fire(), ih.get_joystick_port_b_fire());
}

// --- MemorySystem Game PIA joystick wiring tests ---

TEST(JoystickUnit, GamePIANullInputHandlerFallback) {
    MemorySystem mem;
    // No input handler set — should return hardcoded values
    // Write CRA to enable data mode for Port A
    mem.write(0xA7CE, 0x00);  // CRA = 0 (DDR mode)
    mem.write(0xA7CC, 0x00);  // DDRA = 0x00 (all input)
    mem.write(0xA7CE, 0x04);  // CRA = 0x04 (data mode)
    uint8_t port_a = mem.read(0xA7CC);
    EXPECT_EQ(port_a, 0xFF);  // Fallback: all released

    // Write CRB to enable data mode for Port B
    mem.write(0xA7CF, 0x00);  // CRB = 0 (DDR mode)
    mem.write(0xA7CD, 0x00);  // DDRB = 0x00 (all input)
    mem.write(0xA7CF, 0x04);  // CRB = 0x04 (data mode)
    uint8_t port_b = mem.read(0xA7CD);
    EXPECT_EQ(port_b, 0xC0);  // Fallback: fire buttons released
}

TEST(JoystickUnit, GamePIAReadsJoystickDirections) {
    MemorySystem mem;
    InputHandler ih;
    mem.set_input_handler(&ih);

    // Configure Port A as all-input
    mem.write(0xA7CE, 0x00);  // CRA DDR mode
    mem.write(0xA7CC, 0x00);  // DDRA = 0x00
    mem.write(0xA7CE, 0x04);  // CRA data mode

    ih.set_joystick_direction(0, true, false, false, true);  // joy1: up + right
    uint8_t port_a = mem.read(0xA7CC);
    // bit 0 (up) = 0, bit 3 (right) = 0, rest = 1 → 0xF6
    EXPECT_EQ(port_a, 0xF6);
}

TEST(JoystickUnit, GamePIAReadsJoystickFire) {
    MemorySystem mem;
    InputHandler ih;
    mem.set_input_handler(&ih);

    // Configure Port B as all-input
    mem.write(0xA7CF, 0x00);  // CRB DDR mode
    mem.write(0xA7CD, 0x00);  // DDRB = 0x00
    mem.write(0xA7CF, 0x04);  // CRB data mode

    ih.set_joystick_fire(0, true);
    uint8_t port_b = mem.read(0xA7CD);
    // bit 6 cleared (joy1 fire), bit 7 set → 0x80
    EXPECT_EQ(port_b, 0x80);
}

TEST(JoystickUnit, GamePIADDRAllOutputIgnoresJoystick) {
    MemorySystem mem;
    InputHandler ih;
    mem.set_input_handler(&ih);

    // Configure Port A as all-output
    mem.write(0xA7CE, 0x00);  // CRA DDR mode
    mem.write(0xA7CC, 0xFF);  // DDRA = 0xFF (all output)
    mem.write(0xA7CE, 0x04);  // CRA data mode
    mem.write(0xA7CC, 0xAA);  // ORA = 0xAA

    ih.set_joystick_direction(0, true, true, true, true);
    ih.set_joystick_direction(1, true, true, true, true);
    uint8_t port_a = mem.read(0xA7CC);
    // All output → reads back ORA regardless of joystick
    EXPECT_EQ(port_a, 0xAA);
}
