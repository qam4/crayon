# Implementation Plan: Joystick Emulation

## Overview

Wire the MO5 Game PIA joystick input pins to actual host controller state across three layers: InputHandler (joystick state storage), MemorySystem (Game PIA read logic), and frontends (RetroPad + SDL mapping). Includes port-swap option and save state support.

## Tasks

- [x] 1. Add JoystickState struct and extend InputHandler
  - [x] 1.1 Add JoystickState struct and joystick members to InputHandler
    - Add `JoystickState` struct to `include/input_handler.h` with bool fields: up, down, left, right, fire
    - Add `JoystickState joy_[2]` private member to `InputHandler`
    - Add `JoystickState joy[2]` to `InputState` struct
    - Add public methods: `set_joystick_direction(int port, bool up, bool down, bool left, bool right)`, `set_joystick_fire(int port, bool pressed)`, `get_joystick_port_a() const`, `get_joystick_port_b_fire() const`, `reset_joystick()`
    - _Requirements: 1.1, 1.2, 1.3, 1.4, 1.5_

  - [x] 1.2 Implement joystick methods in input_handler.cpp
    - Implement `set_joystick_direction`: bounds-check port (0 or 1), set direction bools
    - Implement `set_joystick_fire`: bounds-check port, set fire bool
    - Implement `get_joystick_port_a`: build active-low byte from joy_[0] bits 0–3 and joy_[1] bits 4–7
    - Implement `get_joystick_port_b_fire`: return 0xC0 with bits cleared for pressed fire buttons
    - Implement `reset_joystick`: set all fields in joy_[0] and joy_[1] to false
    - Call `reset_joystick()` from `reset()`
    - Extend `get_state()` / `set_state()` to copy joy_ ↔ InputState::joy
    - _Requirements: 1.2, 1.3, 1.4, 1.5_

  - [ ]* 1.3 Write property test: Joystick state round-trip (Property 1)
    - **Property 1: Joystick state round-trip**
    - Generate random port (0/1), random direction/fire booleans; set via `set_joystick_direction`/`set_joystick_fire`, read back via `get_state()`, verify match
    - **Validates: Requirements 1.2**

  - [ ]* 1.4 Write property test: Port A active-low encoding (Property 2)
    - **Property 2: Port A active-low encoding**
    - Generate random 8 direction booleans (4 per port); set them, call `get_joystick_port_a()`, verify each bit is 0 iff corresponding direction is pressed
    - **Validates: Requirements 1.3, 2.4**

  - [ ]* 1.5 Write property test: Port B fire active-low encoding (Property 3)
    - **Property 3: Port B fire active-low encoding**
    - Generate random 2 fire booleans; set them, call `get_joystick_port_b_fire()`, verify bit 6 cleared iff port1 fire pressed, bit 7 cleared iff port2 fire pressed, bits 0–5 always zero
    - **Validates: Requirements 1.4, 2.4**

  - [ ]* 1.6 Write property test: Reset clears all joystick state (Property 4)
    - **Property 4: Reset clears all joystick state**
    - Generate random full joystick state, set it, call `reset()`, verify all directions/fire false and `get_joystick_port_a()` == 0xFF, `get_joystick_port_b_fire()` == 0xC0
    - **Validates: Requirements 1.5**

- [x] 2. Wire Game PIA reads to InputHandler joystick state
  - [x] 2.1 Add set_input_handler to MemorySystem
    - Add `void set_input_handler(InputHandler* ih)` method and `InputHandler* input_handler_ = nullptr` member to `include/memory_system.h`
    - Implement in `src/memory_system.cpp`
    - Wire in `src/emulator_core.cpp` constructor alongside existing `memory_.set_pia(&pia_)` calls: `memory_.set_input_handler(&input_)`
    - _Requirements: 2.1, 2.2_

  - [x] 2.2 Update game_pia_read to use InputHandler joystick state
    - In `game_pia_read` case 0 (Port A): replace hardcoded `0xFF` input_pins with `input_handler_ ? input_handler_->get_joystick_port_a() : 0xFF`
    - In `game_pia_read` case 2 (Port B): replace hardcoded `0xC0` input_pins with `input_handler_ ? input_handler_->get_joystick_port_b_fire() : 0xC0` for bits 6–7, preserving bits 0–5 as zero (DAC feedback)
    - DDR masking formula unchanged: `(OR & DDR) | (input_pins & ~DDR)`
    - _Requirements: 2.1, 2.2, 2.3, 2.4, 6.2, 6.3_

  - [ ]* 2.3 Write property test: Game PIA Port A DDR masking (Property 5)
    - **Property 5: Game PIA Port A DDR masking with joystick input**
    - Generate random DDR, ORA, and joystick direction state; configure Game PIA registers, read Port A, verify `(ORA & DDRA) | (joystick_port_a & ~DDRA)`
    - **Validates: Requirements 2.1, 2.3**

  - [ ]* 2.4 Write property test: Game PIA Port B DAC + fire (Property 6)
    - **Property 6: Game PIA Port B preserves DAC bits and reads fire inputs**
    - Generate random DDR, ORB (DAC value), and fire state; configure Game PIA, read Port B, verify `(ORB & DDRB) | (input_pins & ~DDRB)` where input_pins bits 6–7 come from joystick fire
    - **Validates: Requirements 2.2, 6.2, 6.3**

  - [ ]* 2.5 Write property test: DAC write path unchanged (Property 7)
    - **Property 7: DAC write path unchanged by joystick emulation**
    - Generate random Port B write value and fire state; write to Port B with CRB bit 2 set, verify AudioSystem DAC receives `(value & 0x3F)` scaled correctly regardless of fire state
    - **Validates: Requirements 6.1**

- [x] 3. Checkpoint
  - Ensure all tests pass, ask the user if questions arise.

- [x] 4. Add libretro RetroPad joystick mapping and port swap option
  - [x] 4.1 Add process_joystick_input() to libretro.cpp
    - Add `static bool g_joystick_port_swap = false`
    - Implement `process_joystick_input()`: loop over RetroPad ports 0–1, read D-pad + button B via `input_state_cb`, apply port swap, call `set_joystick_direction` / `set_joystick_fire` on InputHandler
    - Call `process_joystick_input()` from `retro_run()` after `process_retropad_input()` — joystick input is always sampled, even when VKB is visible
    - _Requirements: 3.1, 3.2, 3.3, 3.4, 3.5_

  - [x] 4.2 Add crayon_joystick_port_swap core option and input descriptors
    - Add `crayon_joystick_port_swap` to `core_options[]` with values "disabled" (default) / "enabled"
    - Read option in `poll_core_options()`, call `reset_joystick()` on change before applying new swap value
    - Add port 1 input descriptors for Joy2 (Up, Down, Left, Right, Fire) to the input descriptors array
    - _Requirements: 5.1, 5.2, 5.3, 5.5_

  - [ ]* 4.3 Write unit tests for libretro joystick mapping
    - Test that `process_joystick_input` correctly maps RetroPad state to InputHandler joystick state with swap disabled and enabled
    - Mock `input_state_cb` to provide controlled RetroPad input
    - **Property 8: RetroPad to MO5 joystick mapping**
    - **Property 9: Port swap inverts routing**
    - **Property 10: Port swap toggle clears joystick state**
    - **Validates: Requirements 3.1, 3.2, 3.3, 5.2, 5.3, 5.5**

- [x] 5. Add SDL frontend joystick mapping
  - [x] 5.1 Add joystick input handling to frontend_sdl.cpp
    - In `process_input()` or a new helper, map SDL gamepad 0 D-pad + fire button → MO5 joystick port 1 via `set_joystick_direction` / `set_joystick_fire`
    - Map SDL gamepad 1 (if connected) → MO5 joystick port 2
    - Add keyboard fallback: arrow keys → joy1 directions, Right Ctrl → joy1 fire (OR'd with gamepad input)
    - Apply port swap toggle (F9 or menu option) with `reset_joystick()` on change
    - _Requirements: 4.1, 4.2, 4.3, 4.4, 5.4, 5.5_

- [x] 6. Extend save state serialization for joystick state
  - [x] 6.1 Update savestate.cpp for JoystickState
    - In `write_input`: serialize `joy[0]` and `joy[1]` (5 bools each, 10 bools total) after existing key data
    - In `read_input`: deserialize joystick bools after keys; if data is exhausted (old save state), leave joy[] default-initialized (all false) for backward compatibility
    - _Requirements: 7.1, 7.2, 7.3_

  - [ ]* 6.2 Write property test: Save state joystick round-trip (Property 12)
    - **Property 12: Save state joystick round-trip**
    - Generate random `InputState` with arbitrary keyboard and joystick state; serialize via `write_input`, deserialize via `read_input`, verify joystick state matches
    - **Validates: Requirements 7.1, 7.2**

- [x] 7. Update CMakeLists.txt and add test files
  - Add `tests/test_joystick.cpp` and `tests/test_joystick_props.cpp` to `crayon_tests` target in CMakeLists.txt
  - Ensure test files compile and link against `crayon_core`, `gtest_main`, and `rapidcheck`
  - _Requirements: all_

- [x] 8. Final checkpoint
  - Ensure all tests pass, ask the user if questions arise.

## Notes

- Tasks marked with `*` are optional and can be skipped for faster MVP
- The design uses C++ throughout; RapidCheck is already available via CMake FetchContent
- Port-swap logic lives in the frontend layer only — InputHandler and MemorySystem are swap-unaware
- Active-low encoding is applied at the PIA read boundary, not stored in JoystickState
- Property tests 8–11 (libretro/SDL frontend mapping) require mocking `input_state_cb` or SDL events
