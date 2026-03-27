# Requirements Document

## Introduction

Rewrite the Virtual Keyboard (VKB) overlay for the Crayon MO5 libretro core. The current implementation uses a grid-based layout with broken cursor navigation and no edge-detected input. The rewrite adopts the Videopac emulator's proven VKB architecture: pixel-positioned keys with explicit directional nav links, procedural rendering with a polished visual style, edge-detected D-pad input, and touch/pointer hit testing. The VKB renders directly into the 320×200 XRGB8888 framebuffer with no SDL dependency.

## Glossary

- **VKB**: Virtual Keyboard — an on-screen keyboard overlay rendered into the emulator framebuffer
- **Framebuffer**: The 320×200 pixel XRGB8888 buffer that the libretro core presents each frame
- **VKBKey**: A struct describing one key on the virtual keyboard, including pixel position, dimensions, MO5 scancode, label, and navigation links to adjacent keys
- **Nav_Link**: Per-key indices (up, down, left, right) pointing to the adjacent VKBKey for D-pad cursor movement
- **Cursor**: The currently highlighted key on the VKB, visually distinguished by a yellow background
- **Edge_Detection**: Input technique where an action triggers only on the frame a button transitions from released to pressed, not while held
- **Hit_Test**: Mapping a pointer/touch coordinate to the VKBKey underneath it
- **MO5Key**: Enum of 58 scancodes (0x00–0x39) representing every key on the Thomson MO5 keyboard
- **AZERTY_Layout**: The French keyboard layout used by the Thomson MO5, with rows: digits, AZERTYUIOP, QSDFGHJKLM, WXCVBN
- **Alpha_Blend**: Per-pixel compositing of a foreground color onto a background color using an alpha value
- **Bitmap_Font**: The existing 5×7 pixel font defined in vkeyboard_font.h, covering ASCII 32–127
- **Panel**: The dark background rectangle behind all VKB keys
- **Scheduled_Release**: A key press model where the key is pressed on frame N and automatically released on frame N+1
- **Libretro_Frontend**: Any application implementing the libretro API (RetroArch, Ludo, BizHawk, etc.)

## Requirements

### Requirement 1: Key Data Structure

**User Story:** As a developer, I want each VKB key to be a self-contained struct with pixel position, dimensions, and explicit nav links, so that layout and navigation are decoupled from grid math.

#### Acceptance Criteria

1. THE VKBKey struct SHALL contain fields for: label (const char*), x position (int), y position (int), width (int), height (int), MO5Key scancode, nav_up index (int), nav_down index (int), nav_left index (int), nav_right index (int).
2. WHEN a Nav_Link index equals -1, THE VKB SHALL treat that direction as having no neighbor and keep the Cursor on the current key.
3. THE VKB SHALL define a static array of VKBKey entries containing all 58 MO5 keys in AZERTY_Layout order.

### Requirement 2: MO5 AZERTY Keyboard Layout

**User Story:** As a user, I want the VKB to display all 58 MO5 keys in their authentic AZERTY layout, so that I can find and press any key the MO5 supports.

#### Acceptance Criteria

1. THE VKB layout SHALL contain exactly 58 keys matching every value in the MO5Key enum (scancodes 0x00 through 0x39).
2. THE VKB layout SHALL arrange keys in rows matching the physical MO5 keyboard: Row 0 (STOP, digits 1–9, 0, +, -, ACC, ACC2, EFF), Row 1 (CNT, A, Z, E, R, T, Y, U, I, O, P, *, ENTER), Row 2 (SHIFT, Q, S, D, F, G, H, J, K, L, M, @), Row 3 (BASIC, W, X, C, V, B, N, comma, period, slash, arrows, INS, RAZ), Row 4 (SPACE as a wide key).
3. THE VKB layout SHALL use pixel-positioned keys where each VKBKey specifies absolute x, y, width, and height values rather than row/column grid indices.
4. THE VKB layout SHALL fit within a maximum width of 320 pixels to match the Framebuffer width.

### Requirement 3: Procedural Rendering

**User Story:** As a developer, I want the VKB to render entirely through procedural drawing (filled rectangles and bitmap font), so that no external image assets or SDL dependency are needed.

#### Acceptance Criteria

1. THE VKB SHALL render by writing directly into a caller-provided XRGB8888 Framebuffer using a render(uint32_t* framebuffer, int fb_width, int fb_height) method.
2. THE VKB SHALL draw a Panel background rectangle behind all keys using Alpha_Blend compositing.
3. THE VKB SHALL draw each key as a filled rectangle with a 1-pixel border using Alpha_Blend compositing.
4. THE VKB SHALL draw key labels using the existing 5×7 Bitmap_Font from vkeyboard_font.h, centered within each key face.
5. THE VKB SHALL NOT depend on SDL, external image files, or any library beyond the standard C++ library and the existing Bitmap_Font header.

### Requirement 4: Visual Style

**User Story:** As a user, I want the VKB to have a polished, readable appearance with distinct colors for the panel, keys, cursor, and text, so that keys are easy to identify at a glance.

#### Acceptance Criteria

1. THE VKB SHALL use the following color scheme: Panel background 0xFF1A1A2E, key face 0xFF2D2D44, key border 0xFF4A4A6A, Cursor highlight 0xFFFFCC00 (yellow), normal text 0xFFFFFFFF (white), Cursor text 0xFF000000 (black).
2. WHEN a key is the current Cursor target, THE VKB SHALL render that key with the Cursor highlight color as its face and Cursor text color for its label.
3. WHEN the SHIFT key is active, THE VKB SHALL render the SHIFT key with a distinct visual indicator (Cursor highlight color as its face) regardless of whether the Cursor is on the SHIFT key.

### Requirement 5: Cursor Navigation via D-pad

**User Story:** As a user, I want to move the VKB cursor using the D-pad with predictable, one-step-per-press behavior, so that I can reach any key reliably.

#### Acceptance Criteria

1. WHEN a D-pad direction button transitions from released to pressed (Edge_Detection), THE VKB SHALL move the Cursor to the key indicated by the current key's corresponding Nav_Link.
2. WHILE a D-pad direction button remains held across multiple frames, THE VKB SHALL NOT move the Cursor again until the button is released and pressed again.
3. THE VKB SHALL expose a move_cursor(Direction dir) method accepting an enum Direction with values Up, Down, Left, Right.
4. WHEN the current key's Nav_Link for the pressed direction is -1, THE VKB SHALL keep the Cursor on the current key.

### Requirement 6: Key Press with Scheduled Release

**User Story:** As a user, I want pressing a VKB key to inject a single clean keypress into the emulator, so that games and BASIC interpret exactly one key event per button press.

#### Acceptance Criteria

1. WHEN the confirm button (RetroPad B) transitions from released to pressed while the VKB is visible, THE VKB integration code SHALL set the selected MO5Key to pressed state on the current frame.
2. WHEN a VKB key was pressed on the previous frame via Scheduled_Release, THE VKB integration code SHALL release that MO5Key on the current frame.
3. WHEN the SHIFT toggle is active at the time of a key press, THE VKB integration code SHALL press MO5Key::SHIFT simultaneously with the selected key and release both on the next frame.
4. WHEN a shifted key press completes its Scheduled_Release, THE VKB integration code SHALL clear the SHIFT toggle.

### Requirement 7: Hit Testing for Pointer and Touch Input

**User Story:** As a user on a touch-enabled device, I want to tap directly on VKB keys, so that I can use the keyboard without a gamepad.

#### Acceptance Criteria

1. THE VKB SHALL expose a hit_test(int x, int y) method that returns the index of the VKBKey containing the given Framebuffer coordinate, or -1 if no key is at that position.
2. WHEN a pointer/touch coordinate falls within a VKBKey's bounding rectangle (x, y, width, height) adjusted for the current VKB position offset, THE hit_test method SHALL return that key's index.
3. WHEN a pointer/touch coordinate does not fall within any VKBKey, THE hit_test method SHALL return -1.

### Requirement 8: Position Toggle

**User Story:** As a user, I want to move the VKB between the top and bottom of the screen, so that it does not obscure the part of the game display I need to see.

#### Acceptance Criteria

1. THE VKB SHALL support two positions: Top (VKB aligned to the top of the Framebuffer) and Bottom (VKB aligned to the bottom of the Framebuffer).
2. WHEN toggle_position() is called, THE VKB SHALL switch between Top and Bottom positions.
3. THE VKB SHALL default to the Bottom position on initialization.
4. WHEN the VKB position changes, THE VKB SHALL adjust all rendering and Hit_Test coordinates to reflect the new vertical offset.

### Requirement 9: Configurable Transparency

**User Story:** As a user, I want to adjust VKB transparency, so that I can balance keyboard visibility against seeing the game underneath.

#### Acceptance Criteria

1. THE VKB SHALL support three transparency levels: Opaque (alpha 255), SemiTransparent (alpha 160), and Transparent (alpha 80).
2. WHEN a transparency level is set, THE VKB SHALL apply the corresponding alpha value to all Panel, key face, key border, and text rendering via Alpha_Blend.
3. THE VKB SHALL default to Opaque transparency on initialization.

### Requirement 10: Visibility Toggle

**User Story:** As a user, I want to show and hide the VKB with a single button press, so that it is available when needed and out of the way when not.

#### Acceptance Criteria

1. WHEN toggle_visible() is called, THE VKB SHALL alternate between visible and hidden states.
2. WHILE the VKB is hidden, THE render method SHALL not modify the Framebuffer.
3. WHILE the VKB is hidden, THE VKB integration code SHALL route D-pad input to normal emulator controls instead of VKB navigation.
4. THE VKB SHALL default to hidden on initialization.

### Requirement 11: Libretro Frontend Compatibility

**User Story:** As a developer, I want the VKB to work in any libretro frontend, so that users are not limited to RetroArch.

#### Acceptance Criteria

1. THE VKB class SHALL depend only on a uint32_t* framebuffer pointer and integer dimensions for rendering, with no assumptions about the calling Libretro_Frontend.
2. THE VKB class SHALL NOT call any libretro API functions directly; all input polling and framebuffer management SHALL remain in the libretro integration layer (libretro.cpp).
3. THE VKB integration code in libretro.cpp SHALL use only standard libretro input API (input_state_cb with RETRO_DEVICE_JOYPAD and RETRO_DEVICE_POINTER) for reading user input.

### Requirement 12: Edge-Detected Input in Libretro Integration

**User Story:** As a developer, I want all VKB-related button inputs to be edge-detected in the libretro layer, so that holding a button does not cause repeated actions.

#### Acceptance Criteria

1. THE libretro integration code SHALL track the previous-frame state of each RetroPad button used for VKB control (D-pad directions, B, A, Y, SELECT).
2. WHEN a RetroPad button transitions from released to pressed, THE libretro integration code SHALL trigger the corresponding VKB action exactly once.
3. WHILE a RetroPad button remains held, THE libretro integration code SHALL NOT trigger the corresponding VKB action again.
