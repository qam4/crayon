# Implementation Plan: VKB Rewrite

## Overview

Complete rewrite of the Virtual Keyboard overlay, replacing the grid-based layout with pixel-positioned keys and explicit nav links. The implementation proceeds bottom-up: data structures first, then rendering, then libretro integration, with property tests validating each layer.

## Tasks

- [x] 1. Rewrite VKBKey struct and VirtualKeyboard class header
  - [x] 1.1 Rewrite `include/vkeyboard.h` with new VKBKey struct (label, x, y, width, height, mo5_key, nav_up/down/left/right), Direction enum, VKBPosition/VKBTransparency enums, and VirtualKeyboard class public interface per design
    - Replace grid-based VKBKey (row, col, width) with pixel-positioned struct
    - Add Direction enum, color constants namespace, static LAYOUT[58] declaration, VKB_WIDTH/VKB_HEIGHT constants
    - Change move_cursor signature from (int dx, int dy) to (Direction dir)
    - Add hit_test(int x, int y, int fb_height), get_cursor_index(), get_key_at(int index)
    - Remove cursor_row_, cursor_col_, ROW_COUNT, MAX_COL_COUNT, clamp_cursor(), get_key_index()
    - Add cursor_index_ (int), private rendering helpers: draw_rect, blend_pixel, draw_char, draw_label, get_y_offset
    - Make render() const
    - _Requirements: 1.1, 1.2, 1.3, 2.3, 2.4, 5.3, 7.1, 11.1, 11.2_

- [x] 2. Implement core VKB logic and layout data
  - [x] 2.1 Rewrite `src/vkeyboard.cpp` — define the complete 58-key LAYOUT array with pixel positions and nav links per the design table
    - Each entry: label, x, y, width, height, mo5_key, nav_up, nav_down, nav_left, nav_right
    - Row 0 (15 keys, y=2), Row 1 (15 keys, y=20), Row 2 (14 keys, y=38), Row 3 (13 keys, y=56), Row 4 (SPACE, y=74)
    - Special widths: ENTER 22px, BASIC 40px, SPACE 120px
    - _Requirements: 1.3, 2.1, 2.2, 2.3, 2.4_

  - [x] 2.2 Implement VirtualKeyboard constructor, visibility, shift, position, transparency methods
    - Default state: hidden, bottom, opaque, cursor_index_=0, shift_active_=false
    - toggle_visible, is_visible, toggle_shift, is_shift_active, toggle_position, set_position, get_position, set_transparency, get_transparency
    - _Requirements: 8.2, 8.3, 9.1, 9.3, 10.1, 10.4_

  - [x] 2.3 Implement move_cursor(Direction), press_selected(), get_cursor_index(), get_key_at()
    - move_cursor: look up nav link for direction, update cursor_index_ if not -1
    - press_selected: return LAYOUT[cursor_index_].mo5_key (clamp index to valid range)
    - _Requirements: 1.2, 5.1, 5.3, 5.4, 6.1_

  - [x] 2.4 Implement hit_test(int x, int y, int fb_height)
    - Compute y_offset from position, adjust y, iterate LAYOUT to find containing key
    - Return key index or -1
    - _Requirements: 7.1, 7.2, 7.3, 8.4_

  - [ ]* 2.5 Write property tests for layout and navigation in `tests/test_vkeyboard_props.cpp`
    - **Property 1: Layout geometry invariant** — all keys have positive dimensions and fit within 320×VKB_HEIGHT
    - **Validates: Requirements 2.3, 2.4**
    - **Property 2: Layout covers all MO5 scancodes** — exactly 58 keys, each MO5Key appears exactly once
    - **Validates: Requirements 1.3, 2.1**
    - **Property 3: Nav link validity** — all nav links are -1 or in [0, 57]
    - **Validates: Requirements 1.1, 1.2**
    - **Property 4: Move cursor follows nav links** — for any key and direction with valid nav link, cursor moves to that index
    - **Validates: Requirements 5.1**
    - **Property 5: Move cursor stays on -1 nav link** — cursor unchanged when nav link is -1
    - **Validates: Requirements 1.2, 5.4**
    - **Property 7: No key overlap** — no two keys have overlapping bounding rectangles
    - **Validates: Requirements 7.2**
    - **Property 8: Press selected returns correct key** — press_selected() returns LAYOUT[cursor_index_].mo5_key
    - **Validates: Requirements 6.1**

  - [ ]* 2.6 Write property tests for hit_test and state toggles in `tests/test_vkeyboard_props.cpp`
    - **Property 6: Hit test correctness** — point inside key returns that key's index; point outside all keys returns -1
    - **Validates: Requirements 7.1, 7.2, 7.3**
    - **Property 9: Toggle visible round trip** — two toggles restore original state
    - **Validates: Requirements 10.1**
    - **Property 11: Toggle position round trip** — two toggles restore original state
    - **Validates: Requirements 8.2**
    - **Property 12: Position affects hit test offset** — same key point yields correct index at bottom, -1 at top (when not overlapping)
    - **Validates: Requirements 8.4**

- [x] 3. Checkpoint — Ensure layout and navigation compile and tests pass
  - Ensure all tests pass, ask the user if questions arise.

- [x] 4. Implement rendering pipeline
  - [x] 4.1 Implement private rendering helpers in `src/vkeyboard.cpp`: blend_pixel, draw_rect, get_y_offset
    - blend_pixel: per-channel alpha compositing, XRGB8888
    - draw_rect: fill rectangle at (x + x_offset, y + y_offset), clip to framebuffer bounds
    - get_y_offset: 0 for Top, fb_height - VKB_HEIGHT for Bottom
    - _Requirements: 3.2, 3.3, 9.2_

  - [x] 4.2 Implement draw_char and draw_label in `src/vkeyboard.cpp`
    - draw_char: render 5×7 glyph from FONT_DATA, blend set pixels
    - draw_label: iterate characters with 6px stride, call draw_char
    - _Requirements: 3.4, 3.5_

  - [x] 4.3 Implement render() method in `src/vkeyboard.cpp`
    - Early return if !visible_ or framebuffer==nullptr
    - Compute y_offset and alpha from transparency
    - Draw panel background
    - For each key: determine face color (cursor, shift-active, shift-normal, basic, normal), draw border rect, draw face rect (inset 1px), center and draw label
    - SHIFT key: yellow face, no label text (unless cursor is on it)
    - BASIC key: black face
    - Cursor key: yellow face, black text
    - _Requirements: 3.1, 3.2, 3.3, 3.4, 4.1, 4.2, 4.3, 10.2_

  - [ ]* 4.4 Write property tests for rendering in `tests/test_vkeyboard_props.cpp`
    - **Property 10: Hidden render is no-op** — when not visible, framebuffer unchanged after render()
    - **Validates: Requirements 10.2**
    - **Property 13: Transparency affects blending** — opaque vs transparent produce different pixel values on non-black background
    - **Validates: Requirements 9.2**
    - **Property 14: Cursor highlight rendering** — at least one pixel in cursor key's rect contains cursor color when opaque
    - **Validates: Requirements 4.2**

  - [ ]* 4.5 Write unit tests for rendering edge cases in `tests/test_vkeyboard.cpp`
    - Test default state: hidden, bottom, opaque, cursor at 0
    - Test blend_pixel edge cases: alpha=0 returns bg, alpha=255 returns fg
    - Test SHIFT visual indicator: toggle shift, verify SHIFT key uses highlight color
    - Test specific key positions: STOP at index 0, SPACE at index 57
    - Test ENTER key width > standard, BASIC key width > standard, SPACE key spans multiple units
    - _Requirements: 4.1, 4.3, 8.3, 9.3, 10.4_

- [x] 5. Checkpoint — Ensure rendering compiles and all tests pass
  - Ensure all tests pass, ask the user if questions arise.

- [x] 6. Update libretro integration for edge-detected input and scheduled release
  - [x] 6.1 Update `src/libretro.cpp` — add edge detection state variables and rewrite VKB input handling
    - Add prev_up, prev_down, prev_left, prev_right, prev_b, prev_a, prev_y static bools
    - Replace level-triggered D-pad with edge-detected: `if (cur && !prev) g_vkb.move_cursor(Direction::Up)` etc.
    - Replace level-triggered B/A/Y with edge-detected calls
    - Update move_cursor calls from (dx, dy) to Direction enum
    - _Requirements: 5.1, 5.2, 12.1, 12.2, 12.3_

  - [x] 6.2 Update scheduled release logic in `src/libretro.cpp`
    - Ensure key press on frame N, release on frame N+1
    - Handle SHIFT: press SHIFT+key together, release both next frame, clear shift toggle
    - Verify existing g_vkb_key_pending_release / g_vkb_pending_key variables work with new API
    - _Requirements: 6.1, 6.2, 6.3, 6.4_

- [x] 7. Wire test files into CMakeLists.txt
  - [x] 7.1 Add `tests/test_vkeyboard.cpp` and `tests/test_vkeyboard_props.cpp` to the `crayon_tests` target in `CMakeLists.txt`
    - _Requirements: all (enables test execution)_

- [x] 8. Final checkpoint — Ensure all tests pass and project compiles
  - Ensure all tests pass, ask the user if questions arise.

## Notes

- Tasks marked with `*` are optional and can be skipped for faster MVP
- The VKB class has no SDL dependency — tests run without SDL
- Property tests use RapidCheck (already configured) with Google Test integration
- The 58-key LAYOUT table in the design document is the single source of truth for key positions and nav links
- libretro integration changes are minimal — mostly replacing level-triggered with edge-detected input
