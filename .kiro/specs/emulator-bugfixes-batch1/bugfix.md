# Bugfix Requirements Document — Emulator Bugfixes Batch 1

## Introduction

This document covers four bugs in the Crayon MO5 emulator that affect core emulation accuracy, UI usability, and user workflow. The bugs range from emulation-level issues (cursor blink, cassette loading) to frontend concerns (file browser sizing, config persistence).

## Bug Analysis

---

### Bug 1: Cursor Does Not Blink at BASIC Prompt

#### Current Behavior (Defect)

1.1 WHEN the MO5 boots to the BASIC prompt THEN the cursor is displayed but never blinks, remaining static on screen

1.2 WHEN the ROM's FIRQ handler executes at 50 Hz to toggle cursor pixels in video RAM THEN the toggled pixels are not reflected in the rendered framebuffer because `render_frame()` is called once per frame *before* `signal_vsync()` triggers the FIRQ, so the FIRQ handler's video RAM writes are only rendered in the *next* frame's snapshot — but by then the next FIRQ has toggled them back, making the blink invisible

#### Expected Behavior (Correct)

2.1 WHEN the MO5 boots to the BASIC prompt THEN the cursor SHALL visibly blink at approximately 1-2 Hz (toggling every ~25 frames), matching real MO5 hardware behavior

2.2 WHEN the ROM's FIRQ handler toggles cursor pixels in video RAM THEN the gate array SHALL render those changes in the current frame's output so the toggle is visible to the user

#### Unchanged Behavior (Regression Prevention)

3.1 WHEN the emulator renders a frame with no FIRQ-driven video RAM changes THEN the system SHALL CONTINUE TO render the framebuffer correctly at 50 fps

3.2 WHEN the PIA signals VSYNC THEN the system SHALL CONTINUE TO trigger FIRQ to the CPU once per frame

3.3 WHEN the ROM's FIRQ handler reads PIA port B to acknowledge the interrupt THEN the system SHALL CONTINUE TO clear the FIRQ flag correctly

---

### Bug 2: LOAD"" Cannot Find K7 Cassette Data

#### Current Behavior (Defect)

1.3 WHEN a K7 file is loaded and the user types LOAD"" in BASIC THEN the system displays "searching" but never finds the cassette data, hanging indefinitely

1.4 WHEN the ROM reads cassette data from gate array register $A7C0 bit 7 THEN the cassette bit timing may not match the 1200 baud rate expected by the ROM's bit-reading routine, causing the ROM to miss sync patterns and data bits

1.5 WHEN the cassette interface calculates bit position using elapsed CPU cycles THEN the cycle counter (`current_cycle`) is only updated once per instruction batch in `run_frame()` via `update_cycle(master_clock_.get_cycle_count())`, and the `play_start_cycle` is set from whatever `current_cycle` was at `play()` time, potentially causing timing drift or incorrect bit presentation

#### Expected Behavior (Correct)

2.3 WHEN a valid K7 file is loaded and the user types LOAD"" THEN the system SHALL successfully find and load the cassette data, displaying the program name and completing the load

2.4 WHEN the ROM reads $A7C0 bit 7 during cassette loading THEN the cassette interface SHALL present each data bit for the correct duration at 1200 baud (~833.33 CPU cycles per bit), with proper active-low encoding (0 = signal present, 1 = no signal)

2.5 WHEN the cassette interface tracks timing THEN cycle counting SHALL be accurate relative to the play start, ensuring the ROM's bit-sampling routine reads correct bit values at the expected times

#### Unchanged Behavior (Regression Prevention)

3.4 WHEN no cassette is loaded or playing THEN $A7C0 bit 7 SHALL CONTINUE TO read as 1 (no signal)

3.5 WHEN the cassette is playing but all data has been read THEN the system SHALL CONTINUE TO stop playback and return no-signal

3.6 WHEN the gate array register $A7C0 is written THEN bits 0-4 and 6 SHALL CONTINUE TO be stored correctly (video page, border color)

---

### Bug 3: File Browser Too Large for Default Window

#### Current Behavior (Defect)

1.6 WHEN the file browser is opened in the default 640x400 window (320x200 at 2x scale) THEN the file browser panel was previously hardcoded to 800x500 pixels, overflowing the window bounds

#### Expected Behavior (Correct)

2.6 WHEN the file browser is opened THEN the panel SHALL size itself responsively based on the actual renderer output size using `SDL_GetRendererOutputSize`, fitting within the window with appropriate margins regardless of window dimensions

#### Unchanged Behavior (Regression Prevention)

3.7 WHEN the file browser is opened in a larger or resized window THEN the system SHALL CONTINUE TO display the file browser correctly, scaling to the available space

3.8 WHEN the user navigates files with UP/DOWN/ENTER/BACKSPACE/ESC THEN the system SHALL CONTINUE TO handle input correctly

---

### Bug 4: Config Manager Does Not Remember Last Loaded File Paths

#### Current Behavior (Defect)

1.7 WHEN the user loads a K7 cassette, BASIC ROM, or Monitor ROM via the file browser THEN the system remembers only the directory, not the actual file path that was loaded

1.8 WHEN the emulator is restarted THEN the user must manually re-navigate and re-select the same ROM and cassette files every time, with no option to auto-load previously used files

#### Expected Behavior (Correct)

2.7 WHEN the user loads a K7 cassette, BASIC ROM, or Monitor ROM via the file browser THEN the system SHALL persist the full file path (not just the directory) in the config file for each file type

2.8 WHEN the emulator starts and `auto_load` is enabled in config THEN the system SHALL automatically load the last-used K7, BASIC ROM, and Monitor ROM files if they still exist on disk

2.9 WHEN the emulator starts and `auto_load` is enabled but a previously saved file path no longer exists THEN the system SHALL skip that file gracefully without crashing or showing errors

#### Unchanged Behavior (Regression Prevention)

3.9 WHEN the user loads a file via the file browser THEN the system SHALL CONTINUE TO remember the last-used directory for each file type

3.10 WHEN `auto_load` is disabled (default) THEN the system SHALL CONTINUE TO start without loading any files automatically

3.11 WHEN the config file does not contain last-file-path entries THEN the system SHALL CONTINUE TO function normally with empty/default values
