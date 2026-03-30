#include <gtest/gtest.h>
#include <rapidcheck.h>
#include "input_handler.h"

using namespace crayon;

// Property-based tests for joystick emulation
// These are optional (tasks 1.3–1.6) but the file must exist for CMake.

TEST(JoystickProps, Placeholder) {
    // Placeholder to ensure the test file compiles and links
    InputHandler ih;
    EXPECT_EQ(ih.get_joystick_port_a(), 0xFF);
}
