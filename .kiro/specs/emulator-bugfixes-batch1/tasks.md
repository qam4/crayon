# Implementation Plan

## Bug 1: Cursor Blink

- [ ] 1. Write cursor blink exploration test
  - **Property 1: Fault Condition** - Cursor Blink Visibility
  - **CRITICAL**: This test MUST FAIL on unfixed code - failure confirms the bug exists
  - **DO NOT attempt to fix the test or the code when it fails**
  - **NOTE**: This test encodes the expected behavior - it will validate the fix when it passes after implementation
  - **GOAL**: Surface counterexamples that demonstrate the cursor never blinks
  - **Scoped PBT Approach**: Boot emulator to BASIC prompt, run ~100 frames, capture framebuffer after each frame. Assert that at least some pixel positions change between frames (cursor toggle). On unfixed code, `render_frame()` is called BEFORE `signal_vsync()`, so FIRQ handler writes are never visible in the rendered output.
  - Bug condition from design: `render_frame()` called before `signal_vsync()` AND ROM FIRQ handler toggles video RAM pixels AND toggled pixels not visible in rendered framebuffer
  - Expected behavior: framebuffer SHALL reflect FIRQ-toggled cursor pixels in the same frame, cursor blinks at ~1-2 Hz
  - Run test on UNFIXED code
  - **EXPECTED OUTCOME**: Test FAILS (framebuffer identical across all frames — cursor never blinks)
  - Document counterexamples: e.g., "100 consecutive frames with identical framebuffer despite cursor being present"
  - Mark task complete when test is written, run, and failure is documented
  - _Requirements: 1.1, 1.2, 2.1, 2.2_

- [ ] 2. Write cursor blink preservation tests (BEFORE implementing fix)
  - **Property 2: Preservation** - Frame Rendering Without FIRQ Changes
  - **IMPORTANT**: Follow observation-first methodology
  - Observe on UNFIXED code: run emulator with a simple program (no cursor input), capture framebuffer — verify it renders correctly at 50 fps
  - Observe on UNFIXED code: verify audio sample generation produces expected output
  - Observe on UNFIXED code: verify PIA FIRQ fires once per frame via `signal_vsync()`
  - Write property-based tests: for all frames where no FIRQ-driven video RAM changes occur, the framebuffer output, audio output, and CPU state must be identical before and after the fix
  - Verify tests PASS on UNFIXED code
  - _Requirements: 3.1, 3.2, 3.3_

## Bug 2: K7 Loading

- [ ] 3. Write K7 bit timing exploration test
  - **Property 3: Fault Condition** - K7 Cassette Bit Timing Accuracy
  - **CRITICAL**: This test MUST FAIL on unfixed code - failure confirms the bug exists
  - **DO NOT attempt to fix the test or the code when it fails**
  - **NOTE**: This test encodes the expected behavior - it will validate the fix when it passes after implementation
  - **GOAL**: Surface counterexamples that demonstrate play_start_cycle is stale
  - **Scoped PBT Approach**: Load a known K7 file, advance master clock by N cycles (simulating some emulation), call `play()`, then assert `play_start_cycle == master_clock.get_cycle_count()`. Also read bits at 833-cycle intervals and assert they match expected K7 data.
  - Bug condition from design: `state.playing == true AND state.k7_data is not empty AND state.play_start_cycle != master_clock.get_cycle_count()` — play_start_cycle set from stale `current_cycle` instead of master clock
  - Expected behavior: `play_start_cycle` SHALL equal `master_clock.get_cycle_count()` at time of `play()` call, bits presented at correct 1200 baud timing (~833.33 cycles per bit)
  - Run test on UNFIXED code
  - **EXPECTED OUTCOME**: Test FAILS (play_start_cycle is 0 or stale when master clock is at cycle N >> 0, bit timing is wrong)
  - Document counterexamples: e.g., "play_start_cycle=0 but master_clock=50000, first 60 bits read incorrectly"
  - Mark task complete when test is written, run, and failure is documented
  - _Requirements: 1.3, 1.4, 1.5, 2.3, 2.4, 2.5_

- [ ] 4. Write cassette preservation tests (BEFORE implementing fix)
  - **Property 4: Preservation** - Cassette Interface Non-Playing Behavior
  - **IMPORTANT**: Follow observation-first methodology
  - Observe on UNFIXED code: when no cassette is loaded, read $A7C0 — bit 7 should be 1 (no signal)
  - Observe on UNFIXED code: when cassette is loaded but not playing, read $A7C0 — bit 7 should be 1
  - Observe on UNFIXED code: write to $A7C0 with various values — bits 0-4 and 6 should be stored correctly
  - Write property-based tests: for all states where cassette is NOT playing, $A7C0 bit 7 reads as 1 and other gate array bits are unaffected
  - Verify tests PASS on UNFIXED code
  - _Requirements: 3.4, 3.5, 3.6_

## Bug 4: Config File Path Persistence

- [ ] 5. Write config path persistence exploration test
  - **Property 5: Fault Condition** - File Path Persistence and Auto-Load
  - **CRITICAL**: This test MUST FAIL on unfixed code - failure confirms the bug exists
  - **DO NOT attempt to fix the test or the code when it fails**
  - **NOTE**: This test encodes the expected behavior - it will validate the fix when it passes after implementation
  - **GOAL**: Surface counterexamples that demonstrate file paths are not persisted
  - **Scoped PBT Approach**: Create a ConfigManager, call `set_last_k7_path()` (which doesn't exist yet on unfixed code — test won't compile or the API is missing). Alternatively, simulate a file load action and check config for full path entries — expect only directory saved.
  - Bug condition from design: `(action is file_selected AND full_path NOT persisted to config) OR (action is startup AND auto_load enabled AND no last_file_path entries exist)`
  - Expected behavior: full file path SHALL be persisted per file type, auto-load SHALL load files on restart when `auto_load` enabled
  - Run test on UNFIXED code
  - **EXPECTED OUTCOME**: Test FAILS (API doesn't exist or config has no `last_k7_path` key after file load)
  - Document counterexamples: e.g., "config contains last_k7_dir=/path/to/dir but no last_k7_path entry"
  - Mark task complete when test is written, run, and failure is documented
  - _Requirements: 1.7, 1.8, 2.7, 2.8, 2.9_

- [ ] 6. Write config preservation tests (BEFORE implementing fix)
  - **Property 6: Preservation** - Directory Persistence and Default Startup
  - **IMPORTANT**: Follow observation-first methodology
  - Observe on UNFIXED code: set and get last directories via existing `get/set_last_*_directory()` — verify round-trip works
  - Observe on UNFIXED code: with `auto_load` disabled (default), verify emulator starts without loading any files
  - Observe on UNFIXED code: with no last-file-path entries in config, verify normal startup
  - Write property-based tests: for all configs where `auto_load` is disabled or no file paths saved, startup behavior is unchanged and directory persistence works correctly
  - Verify tests PASS on UNFIXED code
  - _Requirements: 3.9, 3.10, 3.11_

## Implementation

- [ ] 7. Fix Bug 1: Cursor Blink — Reorder run_frame()

  - [ ] 7.1 Reorder run_frame() in src/emulator_core.cpp
    - Move `signal_vsync()` before `render_frame()`
    - After `signal_vsync()`, call `handle_interrupts()` to assert FIRQ to CPU
    - Execute a small bounded loop of CPU instructions (up to ~64) to let the FIRQ handler run (toggle cursor pixels, read PIA Port B, RTI)
    - Then call `render_frame()` so it captures the toggled pixels
    - New order: instructions → signal_vsync → handle_interrupts → execute FIRQ handler → render_frame → audio
    - _Bug_Condition: render_frame() called before signal_vsync(), FIRQ handler writes invisible_
    - _Expected_Behavior: framebuffer reflects FIRQ-toggled cursor pixels in same frame_
    - _Preservation: frame rate 50fps, audio timing, CPU execution, non-cursor rendering unchanged_
    - _Requirements: 2.1, 2.2, 3.1, 3.2, 3.3_

  - [ ] 7.2 Remove premature acknowledge_firq() from handle_interrupts()
    - Remove the automatic `pia_.acknowledge_firq()` call in `handle_interrupts()`
    - Let the FIRQ handler's read of PIA Port B (register 2) naturally clear `irqb1_flag` via existing `read_register(2)` logic
    - Keep auto-acknowledge as fallback AFTER the post-vsync instruction loop runs (for early boot ROM's default RTI-only FIRQ handler that doesn't read Port B)
    - _Bug_Condition: acknowledge_firq() clears flag before CPU FIRQ handler reads PIA Port B_
    - _Expected_Behavior: FIRQ handler reads Port B to acknowledge, clearing flag naturally_
    - _Preservation: PIA FIRQ signaling continues to fire once per frame_
    - _Requirements: 2.2, 3.2, 3.3_

  - [ ] 7.3 Verify cursor blink exploration test now passes
    - **Property 1: Expected Behavior** - Cursor Blink Visibility
    - **IMPORTANT**: Re-run the SAME test from task 1 - do NOT write a new test
    - The test from task 1 encodes the expected behavior (cursor pixels toggle between frames)
    - When this test passes, it confirms the cursor blink is now visible
    - Run cursor blink exploration test from step 1
    - **EXPECTED OUTCOME**: Test PASSES (confirms bug is fixed, cursor blinks)
    - _Requirements: 2.1, 2.2_

  - [ ] 7.4 Verify cursor blink preservation tests still pass
    - **Property 2: Preservation** - Frame Rendering Without FIRQ Changes
    - **IMPORTANT**: Re-run the SAME tests from task 2 - do NOT write new tests
    - Run preservation tests from step 2
    - **EXPECTED OUTCOME**: Tests PASS (confirms no regressions in frame rendering, audio, FIRQ timing)
    - Confirm all tests still pass after fix

- [ ] 8. Fix Bug 2: K7 Loading — Fix play_start_cycle

  - [ ] 8.1 Update play() and rewind() signatures in cassette_interface
    - In `include/cassette_interface.h`: change `play()` to `play(uint64_t current_master_cycle)` and `rewind()` to `rewind(uint64_t current_master_cycle)`
    - In `src/cassette_interface.cpp`: set `state_.play_start_cycle = current_master_cycle` in both methods instead of using `state_.current_cycle`
    - _Bug_Condition: play_start_cycle set from stale current_cycle, not master clock_
    - _Expected_Behavior: play_start_cycle == master_clock.get_cycle_count() at time of play()_
    - _Preservation: non-playing cassette behavior unchanged, $A7C0 bits 0-6 unaffected_
    - _Requirements: 2.3, 2.4, 2.5, 3.4, 3.5, 3.6_

  - [ ] 8.2 Update all call sites for play() and rewind()
    - In `src/frontend_sdl.cpp`: pass `emulator_->get_master_clock().get_cycle_count()` (or add convenience method to EmulatorCore)
    - In `src/emulator_core.cpp`: update any internal calls to pass master clock cycle
    - Consider adding `EmulatorCore::play_cassette()` and `EmulatorCore::rewind_cassette()` convenience methods
    - _Requirements: 2.3, 2.5_

  - [ ] 8.3 Verify K7 bit timing exploration test now passes
    - **Property 3: Expected Behavior** - K7 Cassette Bit Timing Accuracy
    - **IMPORTANT**: Re-run the SAME test from task 3 - do NOT write a new test
    - The test from task 3 encodes the expected behavior (play_start_cycle matches master clock, bits at correct timing)
    - When this test passes, it confirms K7 loading works correctly
    - Run K7 bit timing exploration test from step 3
    - **EXPECTED OUTCOME**: Test PASSES (confirms bug is fixed, bit timing accurate)
    - _Requirements: 2.3, 2.4, 2.5_

  - [ ] 8.4 Verify cassette preservation tests still pass
    - **Property 4: Preservation** - Cassette Interface Non-Playing Behavior
    - **IMPORTANT**: Re-run the SAME tests from task 4 - do NOT write new tests
    - Run preservation tests from step 4
    - **EXPECTED OUTCOME**: Tests PASS (confirms no regressions in non-playing cassette behavior)
    - Confirm all tests still pass after fix

- [ ] 9. Verify Bug 3: File Browser Responsive Sizing
  - Verify file browser renders correctly at default 640×400 window size (320×200 at 2x scale)
  - Verify `render()` uses `SDL_GetRendererOutputSize()` for responsive sizing
  - Verify panel fits within window bounds with appropriate margins
  - No code changes expected — verification only
  - _Requirements: 2.6, 3.7, 3.8_

- [ ] 10. Fix Bug 4: Config File Path Persistence

  - [ ] 10.1 Add file path getter/setter API to ConfigManager
    - In `include/ui/config_manager.h`: add declarations for `get/set_last_k7_path()`, `get/set_last_basic_rom_path()`, `get/set_last_monitor_rom_path()`
    - In `src/ui/config_manager.cpp`: implement getters/setters mapping to INI keys `last_k7_path`, `last_basic_rom_path`, `last_monitor_rom_path` in `[General]` section
    - _Bug_Condition: full file path not persisted to config on file selection_
    - _Expected_Behavior: full path persisted per file type via new API_
    - _Preservation: existing directory persistence unchanged, INI parsing unbroken_
    - _Requirements: 2.7, 3.9, 3.11_

  - [ ] 10.2 Distinguish BASIC ROM vs Monitor ROM in file browser
    - Add `FileType::BasicROM` and `FileType::MonitorROM` enum values (or track pending menu action)
    - Update file browser open calls to use the correct file type so the correct config path is saved on selection
    - _Requirements: 2.7_

  - [ ] 10.3 Save file paths on selection in frontend_sdl.cpp
    - In `process_input()`, after a file is successfully loaded via file browser, call the appropriate `config_manager_->set_last_*_path(selected_path)` and `config_manager_->save()`
    - Handle K7, BASIC ROM, and Monitor ROM file types
    - _Requirements: 2.7_

  - [ ] 10.4 Add auto-load logic in frontend initialize()
    - In `initialize()`, after creating emulator and before `emulator_->reset()`, check `config_manager_->get_auto_load_last_files()`
    - If enabled: load `get_last_basic_rom_path()`, `get_last_monitor_rom_path()`, `get_last_k7_path()` if files exist (`std::filesystem::exists()`)
    - Skip gracefully (no error, no crash) if any file doesn't exist
    - _Bug_Condition: startup with auto_load enabled but no file path entries_
    - _Expected_Behavior: auto-load files that exist, skip missing ones gracefully_
    - _Preservation: when auto_load disabled, no files auto-loaded_
    - _Requirements: 2.8, 2.9, 3.10, 3.11_

  - [ ] 10.5 Verify config path persistence exploration test now passes
    - **Property 5: Expected Behavior** - File Path Persistence and Auto-Load
    - **IMPORTANT**: Re-run the SAME test from task 5 - do NOT write a new test
    - The test from task 5 encodes the expected behavior (file paths persisted, auto-load works)
    - When this test passes, it confirms config persistence is fixed
    - Run config path persistence exploration test from step 5
    - **EXPECTED OUTCOME**: Test PASSES (confirms bug is fixed)
    - _Requirements: 2.7, 2.8, 2.9_

  - [ ] 10.6 Verify config preservation tests still pass
    - **Property 6: Preservation** - Directory Persistence and Default Startup
    - **IMPORTANT**: Re-run the SAME tests from task 6 - do NOT write new tests
    - Run preservation tests from step 6
    - **EXPECTED OUTCOME**: Tests PASS (confirms no regressions in directory persistence and default startup)
    - Confirm all tests still pass after fix

- [ ] 11. Checkpoint - Ensure all tests pass
  - Run all exploration tests (tasks 1, 3, 5) — all should now PASS
  - Run all preservation tests (tasks 2, 4, 6) — all should still PASS
  - Run any existing project tests to verify no regressions
  - Verify Bug 3 file browser sizing manually
  - Ask the user if questions arise
