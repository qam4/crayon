# Requirements Document

## Introduction

The Crayon MO5 emulator has a minimal libretro core (`src/libretro.cpp`) that handles basic frame rendering and cartridge loading. This feature brings the libretro core to production quality, matching the maturity of the author's Videopac libretro core. The work covers: virtual keyboard for gamepad-only users (essential since MO5 games require typing `LOAD""` + `RUN`), RetroPad and physical keyboard input mapping, K7 cassette file detection and loading, core options, BIOS auto-discovery, stereo audio conversion, in-memory save states, light pen pointer mapping, input descriptors, and proper error logging.

## Glossary

- **Libretro_Core**: The shared library (`crayon_libretro.so/.dll`) implementing the libretro API, allowing RetroArch and other libretro frontends to run the Crayon MO5 emulator.
- **Virtual_Keyboard (VKB)**: An on-screen keyboard overlay rendered into the framebuffer, navigable via RetroPad D-pad and buttons, enabling gamepad-only users to type on the MO5's 58-key AZERTY keyboard.
- **RetroPad**: The libretro standard abstract gamepad with D-pad, A/B/X/Y face buttons, L/R shoulders, Start, Select, L3/R3.
- **MO5_Key**: One of the 58 scancodes (0x00–0x39) representing physical keys on the Thomson MO5 AZERTY keyboard, as defined in `input_handler.h`.
- **K7_File**: A Thomson cassette tape image file with `.k7` extension, containing BASIC or machine-code programs in the standard K7 block format.
- **Cartridge_ROM**: A Thomson cartridge image file with `.rom`, `.bin`, or `.mo5` extension, loaded into the cartridge address space.
- **Core_Option**: A configurable setting exposed via `retro_variable` / `RETRO_ENVIRONMENT_SET_VARIABLES` that the user can change from the frontend's options menu.
- **BIOS_ROM**: The MO5 system ROMs (`basic5.rom` / `mo5.rom` for BASIC, and the monitor ROM) required for emulation, loaded from the frontend's system directory.
- **Light_Pen**: The MO5's built-in pointing device, mapped to RETRO_DEVICE_POINTER or RETRO_DEVICE_MOUSE in the libretro API.
- **Input_Descriptor**: Metadata provided via `RETRO_ENVIRONMENT_SET_INPUT_DESCRIPTORS` that tells the frontend which RetroPad buttons the core uses and their labels.
- **Save_State**: A serialized snapshot of the full emulator state (CPU, memory, PIA, gate array, audio, cassette, etc.) used for libretro's `retro_serialize` / `retro_unserialize`.

## Requirements

### Requirement 1: Virtual Keyboard Overlay

**User Story:** As a gamepad-only user, I want an on-screen virtual keyboard so that I can type MO5 commands like `LOAD""` and `RUN` without a physical keyboard.

#### Acceptance Criteria

1. WHEN the user presses the SELECT button on RetroPad, THE Virtual_Keyboard SHALL toggle between visible and hidden states.
2. WHILE the Virtual_Keyboard is visible, THE Libretro_Core SHALL render the MO5's 58-key AZERTY layout as an overlay on top of the emulator framebuffer.
3. WHILE the Virtual_Keyboard is visible, WHEN the user presses D-pad directions on RetroPad, THE Virtual_Keyboard SHALL move the cursor highlight to the adjacent key in the pressed direction.
4. WHILE the Virtual_Keyboard is visible, WHEN the user presses the B button on RetroPad, THE Virtual_Keyboard SHALL send the highlighted key's MO5_Key press and release to the InputHandler.
5. WHILE the Virtual_Keyboard is visible, WHEN the user presses the Y button on RetroPad, THE Virtual_Keyboard SHALL toggle the overlay position between top and bottom of the screen.
6. WHILE the Virtual_Keyboard is visible, WHEN the user presses the A button on RetroPad, THE Virtual_Keyboard SHALL toggle the MO5 SHIFT key state for the next key press.
7. THE Virtual_Keyboard SHALL support a transparency Core_Option with at least three levels (opaque, semi-transparent, transparent).
8. THE Virtual_Keyboard SHALL include all 58 MO5_Key entries including SHIFT, BASIC, CNT, STOP, ENTER, SPACE, arrow keys, and the accent keys.

### Requirement 2: RetroPad Input Mapping

**User Story:** As a RetroArch user, I want RetroPad buttons mapped to MO5 joystick and common keys so that I can play action games without the virtual keyboard.

#### Acceptance Criteria

1. THE Libretro_Core SHALL map RetroPad D-pad Up/Down/Left/Right to MO5_Key UP/DOWN/LEFT/RIGHT.
2. THE Libretro_Core SHALL map RetroPad B button to MO5_Key SPACE (fire/action) when the Virtual_Keyboard is hidden.
3. THE Libretro_Core SHALL map RetroPad A button to MO5_Key ENTER when the Virtual_Keyboard is hidden.
4. THE Libretro_Core SHALL map RetroPad Start button to MO5_Key STOP.
5. THE Libretro_Core SHALL register Input_Descriptors for all mapped RetroPad buttons via RETRO_ENVIRONMENT_SET_INPUT_DESCRIPTORS.

### Requirement 3: Physical Keyboard Pass-Through

**User Story:** As a desktop RetroArch user with a keyboard, I want physical keyboard presses forwarded to the MO5 so that I can type directly.

#### Acceptance Criteria

1. WHEN a RETRO_DEVICE_KEYBOARD key-down event is received, THE Libretro_Core SHALL map the retro_key to the corresponding MO5_Key using the AZERTY mapping table and set the key as pressed in the InputHandler.
2. WHEN a RETRO_DEVICE_KEYBOARD key-up event is received, THE Libretro_Core SHALL release the corresponding MO5_Key in the InputHandler.
3. THE Libretro_Core SHALL handle RETROK_LSHIFT and RETROK_RSHIFT by mapping them to MO5_Key SHIFT and MO5_Key BASIC respectively.

### Requirement 4: K7 Cassette File Loading

**User Story:** As a user, I want to load `.k7` cassette files via `retro_load_game` so that I can play cassette-based MO5 games.

#### Acceptance Criteria

1. WHEN `retro_load_game` receives content with a `.k7` file extension, THE Libretro_Core SHALL load the data into the CassetteInterface instead of the cartridge address space.
2. WHEN `retro_load_game` receives content with a `.rom`, `.bin`, or `.mo5` extension, THE Libretro_Core SHALL load the data as a Cartridge_ROM.
3. WHEN a K7_File is loaded, THE Libretro_Core SHALL automatically call `play_cassette()` so the tape is ready for the MO5's `LOAD""` command.
4. IF `retro_load_game` receives content with an unrecognized extension, THEN THE Libretro_Core SHALL attempt to detect the format by inspecting the data header and log a warning via the retro log callback.

### Requirement 5: Core Options

**User Story:** As a user, I want configurable options in the RetroArch menu so that I can adjust cassette speed, VKB appearance, and other emulation settings.

#### Acceptance Criteria

1. THE Libretro_Core SHALL expose a "Cassette Load Mode" Core_Option with values "Fast" (default) and "Slow (Real-time)".
2. THE Libretro_Core SHALL expose a "Virtual Keyboard Transparency" Core_Option with values "Opaque" (default), "Semi-Transparent", and "Transparent".
3. THE Libretro_Core SHALL expose a "Virtual Keyboard Position" Core_Option with values "Bottom" (default) and "Top".
4. WHEN a Core_Option value changes, THE Libretro_Core SHALL apply the new setting within the same frame by checking `RETRO_ENVIRONMENT_GET_VARIABLE_UPDATE` in `retro_run`.

### Requirement 6: BIOS Auto-Discovery

**User Story:** As a user, I want the core to automatically find MO5 BIOS ROMs in my RetroArch system directory so that I do not need to rename files manually.

#### Acceptance Criteria

1. WHEN loading BIOS_ROM files, THE Libretro_Core SHALL search the system directory for the BASIC ROM using the following filenames in order: "basic5.rom", "mo5_basic.rom", "mo5.bas".
2. WHEN loading BIOS_ROM files, THE Libretro_Core SHALL search the system directory for the monitor ROM using the following filenames in order: "mo5.rom", "mo5_monitor.rom", "mo5.mon".
3. IF no BIOS_ROM file is found for either the BASIC or monitor ROM, THEN THE Libretro_Core SHALL log an error message listing the searched filenames and return false from `retro_load_game`.
4. THE Libretro_Core SHALL also search a "crayon" subdirectory within the system directory (e.g., `system/crayon/basic5.rom`).

### Requirement 7: Stereo Audio Conversion

**User Story:** As a user, I want correct audio output so that the MO5 buzzer sound plays properly through RetroArch.

#### Acceptance Criteria

1. THE Libretro_Core SHALL convert the AudioSystem's mono samples to stereo interleaved format (left, right, left, right...) before passing them to `retro_audio_sample_batch`.
2. THE Libretro_Core SHALL produce exactly `48000 / 50 = 960` stereo sample frames per call to `retro_run`.
3. THE Libretro_Core SHALL duplicate each mono sample into both the left and right channels of the stereo output.
4. IF the AudioSystem ring buffer contains fewer samples than required, THEN THE Libretro_Core SHALL pad the remaining stereo frames with silence (zero values).

### Requirement 8: In-Memory Save State Serialization

**User Story:** As a user, I want save states to work reliably and quickly so that I can use RetroArch's save/load state, rewind, and netplay features.

#### Acceptance Criteria

1. THE Libretro_Core SHALL serialize emulator state directly into the buffer provided by `retro_serialize` without creating temporary files.
2. THE Libretro_Core SHALL deserialize emulator state directly from the buffer provided by `retro_unserialize` without creating temporary files.
3. THE `retro_serialize_size` function SHALL return a size that accounts for the full SaveState including variable-length cassette data.
4. FOR ALL valid emulator states, serializing via `retro_serialize` then deserializing via `retro_unserialize` SHALL produce an emulator state equivalent to the original (round-trip property).

### Requirement 9: Light Pen Pointer Mapping

**User Story:** As a user, I want to use a mouse or touch input as the MO5 light pen so that I can play light-pen games.

#### Acceptance Criteria

1. THE Libretro_Core SHALL read RETRO_DEVICE_POINTER X and Y coordinates each frame and convert them to MO5 screen coordinates (0–319 horizontal, 0–199 vertical).
2. WHEN the pointer is within the MO5 display area, THE Libretro_Core SHALL update the LightPen position via `set_mouse_position`.
3. WHEN RETRO_DEVICE_POINTER pressed state is active, THE Libretro_Core SHALL set the LightPen button as pressed.

### Requirement 10: Error Logging

**User Story:** As a developer or user, I want the core to report errors and warnings through RetroArch's log interface so that I can diagnose problems.

#### Acceptance Criteria

1. THE Libretro_Core SHALL obtain the `retro_log_printf_t` callback via `RETRO_ENVIRONMENT_GET_LOG_INTERFACE` during `retro_set_environment`.
2. WHEN a BIOS_ROM fails to load, THE Libretro_Core SHALL log an error-level message identifying the missing ROM and searched paths.
3. WHEN a K7_File or Cartridge_ROM fails to parse, THE Libretro_Core SHALL log an error-level message with the failure reason.
4. WHEN a Core_Option value is applied, THE Libretro_Core SHALL log an info-level message with the option name and new value.
5. IF the log callback is not available, THEN THE Libretro_Core SHALL silently skip logging without crashing.
