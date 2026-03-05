# Implementation Plan: Libretro Integration

## Overview

Bring the Crayon MO5 libretro core from its minimal ~150-line state to production quality. Implementation proceeds bottom-up: first the standalone VirtualKeyboard class (new files), then libretro header extensions, then SaveStateManager buffer API, then the libretro.cpp rewrite covering BIOS discovery, K7 detection, input pipeline, audio conversion, core options, and VKB overlay compositing. Finally, wire the CMake build and verify CI compatibility.

## Tasks

- [x] 1. Create VirtualKeyboard class
  - [x] 1.1 Create `include/vkeyboard.h` with VirtualKeyboard class declaration
    - Define `VKBPosition`, `VKBTransparency` enums and `VKBKey` struct
    - Declare the `VirtualKeyboard` class with all public methods: `toggle_visible()`, `is_visible()`, `move_cursor()`, `press_selected()`, `toggle_shift()`, `toggle_position()`, `set_position()`, `set_transparency()`, `get_position()`, `get_selected_key()`, `is_shift_active()`, `render()`
    - Declare the static `LAYOUT[]` array (58 MO5 keys in AZERTY order) and private members
    - Must NOT include any SDL headers — renders directly into XRGB8888 framebuffer
    - _Requirements: 1.1, 1.2, 1.3, 1.4, 1.5, 1.6, 1.7, 1.8_

  - [x] 1.2 Implement VirtualKeyboard in `src/vkeyboard.cpp`
    - Define the static `LAYOUT[]` array with all 58 `VKBKey` entries matching the MO5 AZERTY layout (STOP, digits, letters, SHIFT, BASIC, CNT, ENTER, SPACE, arrows, accent keys)
    - Implement `toggle_visible()`, `toggle_shift()`, `toggle_position()` as simple boolean/enum flips
    - Implement `move_cursor(dx, dy)` with `clamp_cursor()` to keep cursor within grid bounds
    - Implement `get_selected_key()` returning the `MO5Key` at the current cursor position
    - Implement `render()` drawing filled rectangles with text labels onto the XRGB8888 framebuffer, with per-pixel alpha blending via `blend_pixel()` based on transparency setting
    - Implement `set_position()` and `set_transparency()` setters
    - _Requirements: 1.1, 1.2, 1.3, 1.4, 1.5, 1.6, 1.7, 1.8_

  - [ ]* 1.3 Write property test: VKB toggle round-trip
    - **Property 1: VKB toggle round-trip**
    - Generate random VirtualKeyboard states (visible/hidden, position top/bottom, shift on/off), apply the corresponding toggle twice, assert state equals original
    - **Validates: Requirements 1.1, 1.5, 1.6**

  - [ ]* 1.4 Write property test: VKB cursor navigation stays in bounds
    - **Property 2: VKB cursor navigation stays in bounds**
    - Generate random valid (row, col) positions and D-pad directions, call `move_cursor()`, assert resulting position is within grid bounds (0 ≤ row < ROW_COUNT, 0 ≤ col < row's column count)
    - **Validates: Requirements 1.3**

  - [ ]* 1.5 Write property test: VKB selected key matches layout
    - **Property 3: VKB selected key matches layout**
    - Generate random valid (row, col), set cursor there, assert `get_selected_key()` equals `LAYOUT[index].mo5_key`
    - **Validates: Requirements 1.4**

  - [ ]* 1.6 Write unit tests for VirtualKeyboard
    - Test that all 58 MO5Key values appear in the LAYOUT array (Req 1.8)
    - Test each transparency level is stored correctly (Req 1.7)
    - Test SELECT toggle flips visibility (Req 1.1)
    - Test cursor wrapping at grid edges (Req 1.3)
    - _Requirements: 1.7, 1.8_

- [x] 2. Checkpoint — VirtualKeyboard compiles and tests pass
  - Ensure all tests pass, ask the user if questions arise.

- [x] 3. Extend libretro header and add SaveStateManager buffer API
  - [x] 3.1 Add missing constants to `include/libretro.h`
    - Add `RETRO_DEVICE_POINTER` (6) and pointer ID constants (`RETRO_DEVICE_ID_POINTER_X`, `_Y`, `_PRESSED`)
    - Add `retro_key` enum with all key codes needed for MO5 AZERTY mapping (RETROK_RETURN, RETROK_SPACE, RETROK_a–z, RETROK_0–9, RETROK_LSHIFT, RETROK_RSHIFT, RETROK_LCTRL, etc.)
    - Add `RETRO_DEVICE_KEYBOARD` constant if missing
    - _Requirements: 3.1, 3.2, 3.3, 9.1_

  - [x] 3.2 Add buffer-based serialize/deserialize to `SaveStateManager` in `include/savestate.h` and `src/savestate.cpp`
    - Add `static Result<std::vector<uint8_t>> serialize_to_buffer(const SaveState& state)` method
    - Add `static Result<SaveState> deserialize_from_buffer(const uint8_t* data, size_t size)` method
    - Reuse existing `BinaryWriter`/`BinaryReader` logic internally
    - _Requirements: 8.1, 8.2, 8.3_

  - [ ]* 3.3 Write property test: Save state serialization round-trip
    - **Property 9: Save state serialization round-trip**
    - Generate random `SaveState` structs with varying cassette data sizes, serialize to buffer then deserialize, assert equality with original
    - **Validates: Requirements 8.4**

  - [ ]* 3.4 Write property test: Serialize size is sufficient
    - **Property 8: Serialize size is sufficient**
    - Generate random `SaveState` structs, assert that a conservative upper-bound size (512KB) is ≥ actual serialized byte count
    - **Validates: Requirements 8.3**

- [x] 4. Implement BIOS auto-discovery and K7/ROM detection in `src/libretro.cpp`
  - [x] 4.1 Implement BIOS auto-discovery in `retro_load_game`
    - Search system directory for BASIC ROM using filenames in order: "basic5.rom", "mo5_basic.rom", "mo5.bas"
    - Search for monitor ROM using: "mo5.rom", "mo5_monitor.rom", "mo5.mon"
    - Also search `{system_dir}/crayon/` subdirectory for each filename
    - If not found, log error listing all searched filenames and return false
    - _Requirements: 6.1, 6.2, 6.3, 6.4_

  - [x] 4.2 Implement K7 vs cartridge detection in `retro_load_game`
    - Route `.k7` extension to `CassetteInterface` load + `play_cassette()`
    - Route `.rom`, `.bin`, `.mo5` extensions to cartridge loader
    - For unknown extensions, inspect data header for K7 magic bytes, fall back to cartridge, log warning
    - _Requirements: 4.1, 4.2, 4.3, 4.4_

  - [ ]* 4.3 Write property test: File extension routes to correct subsystem
    - **Property 5: File extension routes to correct subsystem**
    - Generate random filenames with `.k7`, `.rom`, `.bin`, `.mo5` extensions, assert correct routing decision
    - **Validates: Requirements 4.1, 4.2**

  - [ ]* 4.4 Write property test: BIOS discovery finds ROM under any accepted filename
    - **Property 6: BIOS discovery finds ROM under any accepted filename**
    - Generate random placements of ROM files under accepted names in temp directories, assert discovery function returns the correct path
    - **Validates: Requirements 6.1, 6.2, 6.4**

  - [ ]* 4.5 Write unit tests for BIOS and K7 detection
    - Test BIOS not found returns false and logs error (Req 6.3)
    - Test K7 auto-play: after loading K7, cassette is playing (Req 4.3)
    - Test unknown extension with valid K7 header routes to cassette (Req 4.4)
    - _Requirements: 4.3, 4.4, 6.3_

- [x] 5. Checkpoint — BIOS discovery and content loading work
  - Ensure all tests pass, ask the user if questions arise.

- [x] 6. Implement core options and error logging in `src/libretro.cpp`
  - [x] 6.1 Register core options in `retro_set_environment`
    - Define `retro_variable` array with "crayon_cassette_mode", "crayon_vkb_transparency", "crayon_vkb_position"
    - Call `RETRO_ENVIRONMENT_SET_VARIABLES` to register them
    - Obtain log callback via `RETRO_ENVIRONMENT_GET_LOG_INTERFACE`
    - _Requirements: 5.1, 5.2, 5.3, 10.1_

  - [x] 6.2 Implement core option polling in `retro_run`
    - Check `RETRO_ENVIRONMENT_GET_VARIABLE_UPDATE` each frame
    - When changed, read each variable via `RETRO_ENVIRONMENT_GET_VARIABLE` and apply: cassette speed mode, VKB transparency, VKB position
    - Log info-level message when an option value changes
    - Guard all log calls with `if (log_cb)` null check
    - _Requirements: 5.4, 10.4, 10.5_

  - [ ]* 6.3 Write unit tests for core options and logging
    - Test core options array contains all 3 options with correct keys and values (Req 5.1–5.3)
    - Test null log callback does not crash (Req 10.5)
    - Test option change emits info-level log (Req 10.4)
    - _Requirements: 5.1, 5.2, 5.3, 10.4, 10.5_

- [x] 7. Implement input pipeline in `src/libretro.cpp`
  - [x] 7.1 Implement RetroPad input mapping and VKB navigation in `retro_run`
    - When VKB hidden: map D-pad to MO5 UP/DOWN/LEFT/RIGHT, B→SPACE, A→ENTER, Start→STOP
    - When VKB visible: route D-pad to `g_vkb.move_cursor()`, B to `press_selected()`, A to `toggle_shift()`, Y to `toggle_position()`
    - SELECT toggles VKB visibility (edge-detected via `g_select_prev`)
    - Register input descriptors via `RETRO_ENVIRONMENT_SET_INPUT_DESCRIPTORS` in `retro_set_environment`
    - _Requirements: 1.1, 1.3, 1.4, 1.5, 1.6, 2.1, 2.2, 2.3, 2.4, 2.5_

  - [x] 7.2 Implement physical keyboard pass-through in `retro_run`
    - Build static RETROK→MO5Key AZERTY mapping table
    - Each frame, poll `RETRO_DEVICE_KEYBOARD` for each mapped key and set/release in InputHandler
    - Map RETROK_LSHIFT→MO5Key::SHIFT, RETROK_RSHIFT→MO5Key::BASIC
    - _Requirements: 3.1, 3.2, 3.3_

  - [x] 7.3 Implement light pen pointer mapping in `retro_run`
    - Read `RETRO_DEVICE_POINTER` X/Y each frame
    - Convert from [-32768, 32767] to MO5 screen coordinates [0,319]×[0,199] with clamping
    - Update LightPen position via `set_mouse_position()` when pointer is in display area
    - Set LightPen button from `RETRO_DEVICE_ID_POINTER_PRESSED` state
    - _Requirements: 9.1, 9.2, 9.3_

  - [ ]* 7.4 Write property test: Keyboard press/release round-trip
    - **Property 4: Keyboard press/release round-trip**
    - Generate random mapped retro_keys, press then release via InputHandler, assert MO5Key returns to unpressed state
    - **Validates: Requirements 3.1, 3.2**

  - [ ]* 7.5 Write property test: Pointer-to-MO5 coordinate mapping is within display bounds
    - **Property 10: Pointer-to-MO5 coordinate mapping is within display bounds**
    - Generate random pointer (x, y) in [-32768, 32767], convert to MO5 coords, assert result is in [0,319]×[0,199]
    - **Validates: Requirements 9.1, 9.2**

  - [ ]* 7.6 Write unit tests for input mapping
    - Test RetroPad D-pad→arrow keys, B→SPACE, A→ENTER, Start→STOP (Req 2.1–2.4)
    - Test RETROK_LSHIFT→SHIFT, RETROK_RSHIFT→BASIC (Req 3.3)
    - Test input descriptors array contains entries for all mapped buttons (Req 2.5)
    - _Requirements: 2.1, 2.2, 2.3, 2.4, 2.5, 3.3_

- [x] 8. Checkpoint — Input pipeline complete
  - Ensure all tests pass, ask the user if questions arise.

- [x] 9. Implement audio conversion and save states in `src/libretro.cpp`
  - [x] 9.1 Implement mono-to-stereo audio conversion in `retro_run`
    - Read mono samples from AudioSystem (up to 960 per frame)
    - Duplicate each mono sample into left and right channels of `g_stereo_buf[960*2]`
    - Pad remaining frames with silence (zero) if fewer than 960 samples available
    - Submit via `audio_batch_cb(g_stereo_buf, 960)`
    - _Requirements: 7.1, 7.2, 7.3, 7.4_

  - [x] 9.2 Implement in-memory save state serialization
    - `retro_serialize_size()`: compute and cache size at load time based on SaveState + cassette data, use 512KB upper bound
    - `retro_serialize()`: collect SaveState from EmulatorCore, call `SaveStateManager::serialize_to_buffer()`, copy into provided buffer
    - `retro_unserialize()`: call `SaveStateManager::deserialize_from_buffer()` from provided buffer, apply to EmulatorCore
    - Remove the temp-file hack from current implementation
    - Log error on failure, return false without corrupting emulator state
    - _Requirements: 8.1, 8.2, 8.3, 10.3_

  - [ ]* 9.3 Write property test: Mono-to-stereo conversion preserves samples
    - **Property 7: Mono-to-stereo conversion preserves samples**
    - Generate random mono buffers of length 0–960, convert to stereo, assert `stereo[i*2] == stereo[i*2+1] == mono[i]` for i < N and `stereo[i*2] == stereo[i*2+1] == 0` for i ≥ N
    - **Validates: Requirements 7.1, 7.2, 7.3, 7.4**

  - [ ]* 9.4 Write unit tests for audio and save states
    - Test stereo output has exactly 960 frames (Req 7.2)
    - Test silence padding when fewer than 960 mono samples (Req 7.4)
    - Test serialize then unserialize round-trip produces equivalent state (Req 8.4)
    - _Requirements: 7.2, 7.4, 8.4_

- [x] 10. Wire VKB overlay compositing and finalize `retro_run`
  - [x] 10.1 Composite VKB overlay onto framebuffer in `retro_run`
    - After `emulator->run_frame()`, if VKB is visible, copy framebuffer to a local buffer and call `g_vkb.render()` on the copy
    - Submit the composited buffer via `video_cb()`
    - When VKB is hidden, submit the original framebuffer directly
    - _Requirements: 1.2_

  - [x] 10.2 Wire VKB key presses to InputHandler
    - When `press_selected()` is called, get the selected MO5Key and call `input_handler.set_key_state(key, true)` then schedule release next frame
    - If shift is active, also press MO5Key::SHIFT, then clear shift after the key press
    - _Requirements: 1.4, 1.6_

- [x] 11. Update CMake build and add new source files
  - [x] 11.1 Add `src/vkeyboard.cpp` to the libretro target in `CMakeLists.txt`
    - Add `src/vkeyboard.cpp` to the `crayon_libretro` shared library sources (or to `crayon_core` if VKB is reusable)
    - Ensure `include/vkeyboard.h` is accessible via include path
    - Ensure the VKB files do NOT depend on SDL (no `ENABLE_SDL` guard needed since they don't include SDL)
    - Add new test files to `crayon_tests` target
    - _Requirements: 1.2_

- [x] 12. Final checkpoint — Full integration
  - Ensure all tests pass, ask the user if questions arise.
  - Verify the libretro core builds as a shared library with `cmake --build --preset dev-mingw`
  - Verify tests pass with `ctest --preset dev-mingw`

## Notes

- Tasks marked with `*` are optional and can be skipped for faster MVP
- Each task references specific requirements for traceability
- Checkpoints ensure incremental validation
- Property tests validate universal correctness properties from the design document (Properties 1–10)
- Unit tests validate specific examples and edge cases
- The VirtualKeyboard must NOT depend on SDL — it renders directly into the XRGB8888 framebuffer
- All new code uses C++17, consistent with the project's `CMAKE_CXX_STANDARD`
- RapidCheck is already configured in CMakeLists.txt via FetchContent
