# Emulator Bugfixes Batch 1 — Bugfix Design

## Overview

This design addresses four bugs in the Crayon MO5 emulator:

1. **Cursor Does Not Blink**: The `run_frame()` loop renders the framebuffer before signaling VSYNC, so the ROM's FIRQ handler (which toggles cursor pixels) never has its writes visible — each toggle is undone by the next frame's FIRQ before rendering.
2. **LOAD"" Cannot Find K7 Data**: The cassette interface sets `play_start_cycle` from its own `current_cycle` field, which may be stale (not yet updated to the master clock's actual cycle count), causing bit timing drift that prevents the ROM from finding sync patterns.
3. **File Browser Too Large**: Already fixed — `render()` uses `SDL_GetRendererOutputSize()` for responsive sizing. Needs verification only.
4. **Config Manager Does Not Remember File Paths**: Only directories are persisted, not full file paths. No auto-load logic exists for previously used files on startup.

## Glossary

- **Bug_Condition (C)**: The specific condition that triggers each bug
- **Property (P)**: The desired correct behavior when the bug condition holds
- **Preservation**: Existing behaviors that must remain unchanged by each fix
- **run_frame()**: The function in `src/emulator_core.cpp` that executes one frame of emulation (CPU instructions, rendering, vsync, audio)
- **signal_vsync()**: PIA method that sets `irqb1_flag = true`, causing FIRQ to fire on the next `handle_interrupts()` call
- **acknowledge_firq()**: PIA method that clears `irqb1_flag` — currently called immediately in `handle_interrupts()` when FIRQ is active
- **read_data_bit()**: CassetteInterface method that calculates which bit to present based on elapsed cycles since play start
- **play_start_cycle**: The CPU cycle count captured when `play()` is called — used as the timing reference for bit presentation
- **K7 format**: Raw dump of MO5 cassette data stream at 1200 baud, including leader tones, sync bytes, and data blocks

## Bug Details

### Bug 1: Cursor Blink — Fault Condition

The bug manifests when the MO5 ROM's FIRQ handler toggles cursor pixels in video RAM at 50 Hz. Because `render_frame()` executes before `signal_vsync()`, the FIRQ handler's writes are captured in the *next* frame — but by then the next FIRQ has toggled them back, making the blink invisible.

**Formal Specification:**
```
FUNCTION isBugCondition_CursorBlink(frame)
  INPUT: frame — one iteration of run_frame()
  OUTPUT: boolean

  RETURN render_frame() is called BEFORE signal_vsync()
         AND ROM FIRQ handler toggles video RAM pixels (cursor blink)
         AND toggled pixels are not visible in the rendered framebuffer
END FUNCTION
```

### Bug 2: K7 Loading — Fault Condition

The bug manifests when `play()` sets `play_start_cycle = state_.current_cycle`, but `current_cycle` is only updated by `update_cycle()` calls during instruction execution. If `play()` is called before the first `update_cycle()` of a frame (or from a context where `current_cycle` is stale), the elapsed cycle calculation in `read_data_bit()` produces incorrect bit positions, causing the ROM's cassette routine to miss sync patterns.

**Formal Specification:**
```
FUNCTION isBugCondition_K7Loading(state)
  INPUT: state — CassetteState at the time play() is called
  OUTPUT: boolean

  RETURN state.playing == true
         AND state.k7_data is not empty
         AND state.play_start_cycle != master_clock.get_cycle_count()
         // play_start_cycle was set from stale current_cycle, not the actual master clock
END FUNCTION
```

### Bug 4: Config File Paths — Fault Condition

The bug manifests when the user loads a file via the file browser — only the parent directory is saved to config, not the full file path. On restart with `auto_load` enabled, there is no file path to load from.

**Formal Specification:**
```
FUNCTION isBugCondition_ConfigPaths(action)
  INPUT: action — a file load action via file browser or emulator restart
  OUTPUT: boolean

  RETURN (action is file_selected AND full_path is NOT persisted to config)
         OR (action is startup AND auto_load is enabled
             AND no last_file_path entries exist in config)
END FUNCTION
```

### Examples

**Bug 1:**
- Boot MO5 to BASIC prompt → cursor appears but never blinks (expected: blinks at ~1-2 Hz)
- Run any BASIC program with INPUT statement → cursor is static during input wait

**Bug 2:**
- Load a valid .k7 file, type `LOAD""` → "Searching" displayed indefinitely, never finds data (expected: finds program, displays name, loads)
- The ROM reads $A7C0 bit 7 at precise intervals expecting 1200 baud timing — if `play_start_cycle` is off by even a few hundred cycles, bit boundaries shift and sync detection fails

**Bug 4:**
- Load a K7 file via F6, restart emulator with `auto_load=true` → no file auto-loaded (expected: last K7 auto-loads)
- Load BASIC ROM via F2, restart → must re-navigate to ROM file (expected: auto-loads if `auto_load` enabled)

## Expected Behavior

### Preservation Requirements

**Bug 1 — Unchanged Behaviors:**
- Frame rate must remain at 50 fps (20000 cycles per frame)
- Audio generation timing must not change
- CPU instruction execution within the frame loop must not change
- Games and programs that don't rely on cursor blink must render identically
- PIA FIRQ signaling must continue to fire once per frame

**Bug 2 — Unchanged Behaviors:**
- When no cassette is loaded or playing, $A7C0 bit 7 reads as 1 (no signal)
- When all K7 data has been read, playback stops and returns no-signal
- Gate array register $A7C0 bits 0-6 (video page, border color) must be unaffected
- Cassette recording functionality must be unaffected
- Save state serialization of CassetteState must continue to work

**Bug 3 — Unchanged Behaviors:**
- File browser navigation (UP/DOWN/ENTER/BACKSPACE/ESC) must work correctly
- File filtering by extension must work correctly
- Directory persistence must continue to work

**Bug 4 — Unchanged Behaviors:**
- Directory persistence (get/set_last_*_directory) must continue to work
- When `auto_load` is disabled (default), no files auto-load on startup
- When config file has no last-file-path entries, emulator starts normally
- Manual file loading via file browser must continue to work
- Config INI parsing and writing must not break existing keys

**Scope:**
All inputs and code paths not related to the specific bug conditions above should be completely unaffected. This includes all keyboard input handling, video rendering of non-cursor pixels, cartridge loading, save states, debugger functionality, and all other UI components.

## Hypothesized Root Cause

### Bug 1: Render-Before-VSYNC Ordering

The root cause is the ordering in `run_frame()`:
```cpp
gate_array_.render_frame(memory_.get_pixel_ram(), memory_.get_color_ram());  // renders BEFORE vsync
pia_.signal_vsync();  // FIRQ fires AFTER render
```

The FIRQ handler toggles cursor pixels in video RAM, but since rendering already happened, the toggle is invisible. By the next frame, the FIRQ toggles the pixels back, so the cursor appears permanently in one state.

Additionally, `handle_interrupts()` calls `pia_.acknowledge_firq()` immediately when FIRQ is active, which clears `irqb1_flag` before the CPU's FIRQ handler has a chance to read PIA Port B (which is the hardware mechanism for acknowledging the interrupt). This means the FIRQ handler may not execute properly even if we reorder the calls — we need to ensure the CPU actually runs the FIRQ handler instructions between vsync and render.

### Bug 2: Stale play_start_cycle

In `CassetteInterface::play()`:
```cpp
state_.play_start_cycle = state_.current_cycle;
```

`state_.current_cycle` is updated by `update_cycle(master_clock_.get_cycle_count())` which is called inside the `run_frame()` instruction loop. But `play()` can be called from `SDLFrontend::initialize()` or from `process_input()` — both outside the instruction loop. At those points, `current_cycle` may be 0 (initial value) or stale from the previous frame.

If `play_start_cycle` is 0 but the master clock has already advanced thousands of cycles, the elapsed calculation `current_cycle - play_start_cycle` will be too large, causing `read_data_bit()` to skip past the leader tone and sync bytes, making the ROM unable to find the data.

### Bug 4: Missing File Path Persistence API

The `ConfigManager` has `get/set_last_*_directory()` methods but no `get/set_last_*_path()` methods. The `select_current_file()` in `file_browser.cpp` saves only the directory. The `SDLFrontend::initialize()` has no auto-load logic that reads saved file paths from config.

## Correctness Properties

Property 1: Fault Condition — Cursor Blink Visibility

_For any_ frame where the ROM's FIRQ handler toggles cursor pixels in video RAM, the rendered framebuffer SHALL reflect those toggled pixels in the same frame's output, making the cursor blink visible at approximately 1-2 Hz.

**Validates: Requirements 2.1, 2.2**

Property 2: Preservation — Frame Rendering Without FIRQ Changes

_For any_ frame where no FIRQ-driven video RAM changes occur, the fixed `run_frame()` SHALL produce the same rendered framebuffer as the original function, preserving all non-cursor rendering, frame timing, and audio generation.

**Validates: Requirements 3.1, 3.2, 3.3**

Property 3: Fault Condition — K7 Cassette Bit Timing Accuracy

_For any_ call to `play()` followed by `read_data_bit()` calls during emulation, the bit position calculation SHALL use an accurate `play_start_cycle` derived from the master clock's current cycle count, presenting each K7 data bit for exactly ~833.33 CPU cycles at 1200 baud.

**Validates: Requirements 2.3, 2.4, 2.5**

Property 4: Preservation — Cassette Interface Non-Playing Behavior

_For any_ state where the cassette is not playing (no K7 loaded, or playback stopped), the fixed cassette interface SHALL produce the same results as the original: $A7C0 bit 7 reads as 1 (no signal), and all other gate array register bits are unaffected.

**Validates: Requirements 3.4, 3.5, 3.6**

Property 5: Fault Condition — File Path Persistence and Auto-Load

_For any_ file loaded via the file browser (K7, BASIC ROM, Monitor ROM), the full file path SHALL be persisted to the config file, and on restart with `auto_load` enabled, the emulator SHALL automatically load those files if they exist on disk.

**Validates: Requirements 2.7, 2.8, 2.9**

Property 6: Preservation — Directory Persistence and Default Startup

_For any_ configuration where `auto_load` is disabled or no last-file-path entries exist, the fixed code SHALL produce the same startup behavior as the original, preserving directory persistence and normal startup without auto-loading.

**Validates: Requirements 3.9, 3.10, 3.11**

## Fix Implementation

### Changes Required

#### Bug 1: Cursor Blink — Reorder run_frame()

**File**: `src/emulator_core.cpp`
**Function**: `run_frame()`

**Specific Changes**:

1. **Move `signal_vsync()` before `render_frame()`**: Call `pia_.signal_vsync()` immediately after the instruction execution loop completes, before rendering.

2. **Execute FIRQ handler before rendering**: After `signal_vsync()`, call `handle_interrupts()` to assert FIRQ to the CPU, then call `cpu_.execute_instruction()` in a small loop to let the FIRQ handler run. The FIRQ handler is typically short (reads PIA Port B to acknowledge, toggles cursor pixels, RTI). Running a bounded number of instructions (e.g., up to 64) or until the FIRQ is serviced should suffice.

3. **Remove premature FIRQ acknowledgment**: In `handle_interrupts()`, remove the automatic `pia_.acknowledge_firq()` call. Instead, let the FIRQ handler's read of PIA Port B (register 2, address $A7C1 mapped to PIA reg 2) naturally clear `irqb1_flag` via the existing `read_register(2)` logic which sets `irqb1_flag = false`. This matches real hardware behavior.

   **Caveat**: The current `acknowledge_firq()` exists because the early boot ROM's default FIRQ handler is just RTI (no Port B read), which would cause infinite FIRQ loops. We need to keep the auto-acknowledge but delay it — acknowledge after the FIRQ handler has had a chance to run (after the post-vsync instruction execution), not immediately when FIRQ is asserted.

**New `run_frame()` order**:
```
1. Execute instructions until frame complete (20000 cycles)
2. pia_.signal_vsync()           — set FIRQ flag
3. handle_interrupts()           — assert FIRQ line to CPU
4. Execute a few instructions    — let FIRQ handler run (toggles cursor pixels)
5. gate_array_.render_frame()    — now captures the toggled pixels
6. audio_.generate_samples()
```

#### Bug 2: K7 Loading — Fix play_start_cycle

**File**: `src/cassette_interface.cpp` and `include/cassette_interface.h`
**Function**: `play()`

**Specific Changes**:

1. **Add master clock cycle parameter to `play()`**: Change signature to `void play(uint64_t current_master_cycle)` so the caller passes the authoritative cycle count from the master clock.

2. **Set `play_start_cycle` from parameter**: In `play()`, set `state_.play_start_cycle = current_master_cycle` instead of using the potentially stale `state_.current_cycle`.

3. **Update all call sites**: In `frontend_sdl.cpp` (initialize) and anywhere else `play()` is called, pass `emulator_->get_master_clock().get_cycle_count()` (or equivalent). If the master clock isn't easily accessible from the frontend, add a convenience method to `EmulatorCore` like `play_cassette()` that internally calls `cassette_.play(master_clock_.get_cycle_count())`.

4. **Same fix for `rewind()`**: Apply the same pattern — `rewind()` also sets `play_start_cycle = state_.current_cycle` and has the same stale-cycle problem.

#### Bug 3: File Browser — Verification Only

**File**: `src/ui/file_browser.cpp`

No code changes needed. The `render()` method already uses `SDL_GetRendererOutputSize()` and computes panel dimensions as 90% of window size with margins. Verify this works at the default 640×400 window size.

#### Bug 4: Config File Path Persistence

**File**: `include/ui/config_manager.h`

**Specific Changes**:
1. **Add file path getter/setter declarations**:
   - `std::string get_last_k7_path() const;`
   - `void set_last_k7_path(const std::string& path);`
   - `std::string get_last_basic_rom_path() const;`
   - `void set_last_basic_rom_path(const std::string& path);`
   - `std::string get_last_monitor_rom_path() const;`
   - `void set_last_monitor_rom_path(const std::string& path);`

**File**: `src/ui/config_manager.cpp`

2. **Implement file path getters/setters**: Each maps to a key in the `[General]` section of config.ini:
   - `last_k7_path`, `last_basic_rom_path`, `last_monitor_rom_path`

**File**: `src/frontend_sdl.cpp`

3. **Save file paths on selection**: In `process_input()`, after a file is successfully loaded via the file browser, call the appropriate `config_manager_->set_last_*_path(selected)` and `config_manager_->save()`.

4. **Auto-load on startup**: In `initialize()`, after creating the emulator and before `emulator_->reset()`, check if `config_manager_->get_auto_load_last_files()` is true. If so:
   - Load `get_last_basic_rom_path()` if file exists (`std::filesystem::exists()`)
   - Load `get_last_monitor_rom_path()` if file exists
   - Load `get_last_k7_path()` if file exists, and call `play()`
   - Skip gracefully (no error, no crash) if any file doesn't exist

5. **Distinguish BASIC ROM vs Monitor ROM in file browser**: Currently `LoadBasicROM` and `LoadMonitorROM` both open with `FileType::ROM`. We need a way to distinguish which ROM type was selected so we can save the correct path. Options:
   - Add a `FileType::BasicROM` and `FileType::MonitorROM` enum value
   - Or track the pending menu action in a member variable and check it when the file is selected

## Testing Strategy

### Validation Approach

The testing strategy follows a two-phase approach: first, surface counterexamples that demonstrate the bugs on unfixed code, then verify the fixes work correctly and preserve existing behavior.

### Exploratory Fault Condition Checking

**Goal**: Surface counterexamples that demonstrate the bugs BEFORE implementing fixes. Confirm or refute the root cause analysis.

**Bug 1 Test Plan**: Write a test that boots the emulator for ~100 frames and captures the framebuffer at each frame. Check if any pixel positions change between consecutive frames (indicating cursor blink). On unfixed code, no pixels should change — confirming the blink is invisible.

**Test Cases**:
1. **Cursor Blink Test**: Boot to BASIC prompt, run 100 frames, compare framebuffers — expect NO pixel changes on unfixed code (will fail to find blink)
2. **VSYNC-Render Order Test**: Instrument `run_frame()` to log the order of render vs vsync calls — confirm render happens before vsync on unfixed code

**Bug 2 Test Plan**: Write a test that loads a known K7 file, calls `play()`, then simulates the ROM's bit-reading pattern (reading $A7C0 at 833-cycle intervals). On unfixed code, the bits read should not match the expected K7 data.

**Test Cases**:
3. **K7 Bit Timing Test**: Load K7, call `play()`, read bits at 833-cycle intervals — expect incorrect bit values on unfixed code due to stale `play_start_cycle`
4. **Play Start Cycle Test**: Call `play()` and check `play_start_cycle` vs `master_clock.get_cycle_count()` — expect mismatch on unfixed code

**Bug 4 Test Plan**: Load a file via file browser, check config for full path — expect only directory saved on unfixed code.

**Test Cases**:
5. **Config Path Persistence Test**: Load K7 via browser, read config — expect no `last_k7_path` key on unfixed code
6. **Auto-Load Test**: Set `auto_load=true` with saved paths, restart — expect no auto-load on unfixed code

**Expected Counterexamples**:
- Bug 1: Framebuffer is identical across all 100 frames despite cursor being present
- Bug 2: `play_start_cycle` is 0 when master clock is at cycle N >> 0, causing bit offset
- Bug 4: Config file contains `last_k7_dir` but no `last_k7_path`

### Fix Checking

**Goal**: Verify that for all inputs where each bug condition holds, the fixed functions produce the expected behavior.

**Bug 1 Pseudocode:**
```
FOR ALL frame WHERE ROM FIRQ handler toggles cursor pixels DO
  framebuffer_before := capture_framebuffer()
  run_frame_fixed()
  framebuffer_after := capture_framebuffer()
  ASSERT framebuffer_before != framebuffer_after at cursor position
  // Cursor pixels should toggle every ~25 frames
END FOR
```

**Bug 2 Pseudocode:**
```
FOR ALL play_action WHERE play() is called DO
  play_fixed(master_clock.get_cycle_count())
  ASSERT state.play_start_cycle == master_clock.get_cycle_count()
  FOR bit_index IN 0..k7_data_bits DO
    advance_clock(833 cycles)
    bit := read_data_bit()
    ASSERT bit == expected_k7_bit(bit_index)
  END FOR
END FOR
```

**Bug 4 Pseudocode:**
```
FOR ALL file_load_action WHERE user selects file via browser DO
  result := load_file_fixed(selected_path)
  ASSERT config.get_last_*_path() == selected_path
  restart_emulator_with_auto_load()
  ASSERT file_is_loaded(selected_path)
END FOR
```

### Preservation Checking

**Goal**: Verify that for all inputs where the bug conditions do NOT hold, the fixed functions produce the same results as the original functions.

**Bug 1 Pseudocode:**
```
FOR ALL frame WHERE no FIRQ-driven video RAM changes occur DO
  ASSERT run_frame_original(frame) == run_frame_fixed(frame)
  // Same framebuffer, same audio output, same CPU state
END FOR
```

**Bug 2 Pseudocode:**
```
FOR ALL state WHERE cassette is NOT playing DO
  ASSERT read_A7C0_original(state) == read_A7C0_fixed(state)
  // Bit 7 = 1 (no signal), other bits unchanged
END FOR
```

**Bug 4 Pseudocode:**
```
FOR ALL config WHERE auto_load is disabled OR no file paths saved DO
  ASSERT startup_original(config) == startup_fixed(config)
  // No files auto-loaded, directories still persisted
END FOR
```

**Testing Approach**: Property-based testing is recommended for preservation checking because:
- It generates many test cases automatically across the input domain
- It catches edge cases that manual unit tests might miss
- It provides strong guarantees that behavior is unchanged for all non-buggy inputs

**Test Plan**: Observe behavior on UNFIXED code first for non-bug inputs, then write property-based tests capturing that behavior.

**Test Cases**:
1. **Frame Rendering Preservation**: Run emulator with a simple program (no cursor), verify framebuffer output is identical before and after Bug 1 fix
2. **Cassette No-Signal Preservation**: Verify $A7C0 reads 0x80 (no signal) when no cassette is loaded, before and after Bug 2 fix
3. **Config Directory Preservation**: Verify directory persistence still works after Bug 4 changes
4. **Audio Output Preservation**: Verify audio sample generation is unchanged after Bug 1 reordering

### Unit Tests

- Test `run_frame()` ordering: verify `signal_vsync()` is called before `render_frame()`
- Test `play(master_cycle)` sets `play_start_cycle` correctly
- Test `read_data_bit()` returns correct bits for a known K7 byte sequence
- Test `ConfigManager::get/set_last_k7_path()` round-trips correctly
- Test `ConfigManager::get/set_last_basic_rom_path()` round-trips correctly
- Test `ConfigManager::get/set_last_monitor_rom_path()` round-trips correctly
- Test auto-load skips gracefully when file doesn't exist
- Test auto-load works when file exists and `auto_load` is enabled

### Property-Based Tests

- Generate random frame sequences and verify cursor pixel toggling is visible in rendered output after Bug 1 fix
- Generate random K7 data bytes and verify `read_data_bit()` returns the correct bit at each 833-cycle interval after Bug 2 fix
- Generate random config states (with/without file paths, with/without auto_load) and verify correct startup behavior after Bug 4 fix
- Generate random non-cassette memory reads and verify $A7C0 behavior is preserved

### Integration Tests

- Full boot-to-BASIC-prompt test: verify cursor blinks visually over 200 frames
- Full K7 load test: load a known K7 file, type LOAD"", verify program loads successfully
- Full config round-trip test: load files, save config, restart, verify auto-load works
- File browser test: open browser at default window size, verify panel fits within bounds (Bug 3 verification)
