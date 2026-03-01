# Requirements Document

## Introduction

Crayon is a Thomson MO5 emulator written in modern C++, built by forking the existing Videopac/Odyssey 2 emulator codebase and replacing machine-specific components with MO5 equivalents. The Thomson MO5 is a French 8-bit home computer from 1984 built around the Motorola 6809E CPU at 1 MHz. The architecture maps 1:1 from Videopac: the CPU (Intel 8048 → Motorola 6809E), video chip (Intel 8245 VDC → Thomson EFGJ03L gate array), and memory map are swapped, while shared infrastructure carries over directly. The carried-over infrastructure includes the CMake build system, GitHub Actions CI/CD, Libretro core structure, ImGui debugger UI, save state framework, SDL frontend, input mapping, and GTest/RapidCheck test infrastructure. The primary milestone is booting to the MO5 BASIC 1.0 prompt. Property-based testing with RapidCheck is used extensively to verify CPU instruction correctness and memory mapping behavior.

## Glossary

- **Emulator**: The Crayon Thomson MO5 emulator application as a whole
- **Videopac_Codebase**: The existing Videopac/Odyssey 2 emulator repository from which Crayon is forked, providing shared infrastructure and component architecture
- **CPU_Core**: The Motorola 6809E CPU emulation component responsible for instruction fetch, decode, and execution, replacing the Videopac Intel 8048 CPU implementation
- **Gate_Array**: The EFGJ03L custom gate array emulation component responsible for video rendering using the forme/fond (shape/background) color system, replacing the Videopac Intel 8245 VDC
- **Memory_System**: The component managing the MO5 memory map, including 48KB RAM (32KB user + 16KB video), ROM, and memory-mapped I/O, replacing the Videopac memory map
- **Audio_System**: The component emulating the 1-bit buzzer sound output, replacing the Videopac shift register audio
- **Input_System**: The component handling the MO5 chiclet keyboard matrix scanning, adapted from the Videopac joystick input handling
- **Light_Pen**: The component emulating the crayon optique (light pen) input device, a new MO5-specific component
- **Cassette_Interface**: The component emulating the cassette tape I/O for loading and saving programs in K7 format, a new MO5-specific component
- **PIA**: The Peripheral Interface Adapter (MC6821) providing keyboard scanning, cassette control, light pen input, and interrupt routing at MO5-specific I/O addresses, a new MO5-specific component replacing Videopac I/O handling
- **Frontend**: The SDL2-based windowing, rendering, and audio output layer, carried over from the Videopac_Codebase (src/frontend_sdl.cpp, include/frontend_sdl.h) and adapted for MO5 display resolution and input
- **Build_System**: The CMake-based build configuration and dependency management, carried over from the Videopac_Codebase (CMakeLists.txt) and adapted for Crayon targets
- **Test_Harness**: The Google Test and RapidCheck-based testing framework for unit and property-based tests, carried over from the Videopac_Codebase (tests/ directory)
- **Libretro_Core**: The libretro API implementation enabling Crayon to run as a core in RetroArch and other libretro frontends, carried over from the Videopac_Codebase (src/libretro.cpp, include/libretro.h) with EmulatorCore internals swapped
- **Debugger_UI**: The ImGui-based interactive debugger interface providing CPU state inspection, memory viewing, breakpoints, and disassembly, carried over from the Videopac_Codebase (src/debugger_ui.cpp, include/debugger_ui.h, include/debugger.h)
- **Save_State_Framework**: The serialization framework for saving and restoring complete emulator state, carried over from the Videopac_Codebase (include/savestate.h, src/savestate.cpp)
- **CI_CD_Pipeline**: The GitHub Actions workflow for continuous integration and release automation, carried over from the Videopac_Codebase (.github/ directory)
- **MO5_BASIC**: The Thomson BASIC 1.0 ROM interpreter (12KB at 0xC000-0xEFFF) used as the primary boot target
- **Monitor_ROM**: The 4KB system monitor ROM (0xF000-0xFFFF) containing reset/interrupt vectors and low-level routines
- **Framebuffer**: The pixel buffer representing the current video output frame (320x200 resolution)
- **Master_Clock**: The component managing timing for the externally clocked 6809E at 1 MHz
- **Video_RAM**: The 16KB of RAM dedicated to video, split into 8KB pixel data (0x0000-0x1FFF) and 8KB color attributes (0x2000-0x3FFF)

## Requirements

### Requirement 1: Fork and Adapt Videopac Codebase

**User Story:** As a developer, I want to fork the Videopac emulator codebase and strip Videopac-specific components while retaining shared infrastructure, so that I can build the MO5 emulator on a proven foundation without starting from scratch.

#### Acceptance Criteria

1. THE Emulator SHALL be created by forking the Videopac_Codebase repository and renaming project references from "videopac" to "crayon"
2. THE Emulator SHALL remove all Videopac-specific source files (src/cpu.cpp, src/vdc.cpp, include/cpu.h, include/vdc.h) and replace them with MO5-specific implementations for CPU_Core, Gate_Array, Memory_System, Audio_System, Input_System, PIA, Light_Pen, and Cassette_Interface
3. THE Emulator SHALL retain the shared infrastructure files from the Videopac_Codebase: Build_System (CMakeLists.txt), Frontend (src/frontend_sdl.cpp), Libretro_Core (src/libretro.cpp), Debugger_UI (src/debugger_ui.cpp, include/debugger_ui.h, include/debugger.h), Save_State_Framework (include/savestate.h, src/savestate.cpp), CI_CD_Pipeline (.github/), and Test_Harness (tests/)
4. THE Emulator SHALL adapt the retained Frontend to use MO5 display resolution (320x200) and MO5 input handling instead of Videopac display and joystick input
5. THE Emulator SHALL adapt the retained Libretro_Core to wrap the MO5 EmulatorCore internals instead of the Videopac EmulatorCore
6. THE Emulator SHALL adapt the retained Debugger_UI to display 6809E CPU registers, MO5 memory regions, and 6809 disassembly instead of 8048 CPU state and Videopac memory
7. THE Emulator SHALL adapt the retained Save_State_Framework to serialize and deserialize MO5 component state instead of Videopac component state
8. THE Emulator SHALL adapt the retained Test_Harness to test MO5 components, removing Videopac-specific tests and adding MO5-specific unit and property-based tests
9. THE Emulator SHALL adapt the retained CI_CD_Pipeline to build and test the Crayon project instead of the Videopac project

### Requirement 2: Build System and Project Structure

**User Story:** As a developer, I want to adapt the existing CMake build system from the Videopac codebase, so that I can compile, test, and iterate on the MO5 emulator efficiently.

#### Acceptance Criteria

1. THE Build_System SHALL be adapted from the Videopac_Codebase CMakeLists.txt to produce a standalone executable named "crayon" using CMake version 3.15 or later
2. THE Build_System SHALL compile all source files using the C++17 standard with extensions disabled
3. THE Build_System SHALL enable compiler warnings (-Wall -Wextra -Wpedantic -Werror on GCC/Clang, /W4 on MSVC)
4. THE Build_System SHALL provide a BUILD_TESTS option that fetches Google Test and RapidCheck and builds the test executable
5. THE Build_System SHALL link against SDL2 for the Frontend when the ENABLE_SDL option is enabled
6. THE Build_System SHALL organize source files into separate translation units for each emulated component (CPU_Core, Gate_Array, Memory_System, Audio_System, Input_System, Light_Pen, PIA, Cassette_Interface)
7. IF SDL2 is not found during configuration, THEN THE Build_System SHALL produce a warning and build a headless-only version

### Requirement 3: Motorola 6809E CPU Core

**User Story:** As a developer, I want an accurate Motorola 6809E CPU emulation replacing the Videopac Intel 8048, so that MO5 software executes correctly.

#### Acceptance Criteria

1. THE CPU_Core SHALL implement all 6809 instruction opcodes including the 0x10 and 0x11 extended prefix pages
2. THE CPU_Core SHALL maintain the following registers: A, B (combinable as D), X, Y, U (user stack pointer), S (hardware stack pointer), PC, DP (direct page register), and CC (condition code register)
3. THE CPU_Core SHALL implement all 6809 addressing modes: inherent, immediate, direct, extended, indexed (with all sub-modes including auto-increment, auto-decrement, constant offset, accumulator offset, and indirect variants)
4. THE CPU_Core SHALL set condition code flags (E, F, H, I, N, Z, V, C) correctly for each instruction that modifies them
5. THE CPU_Core SHALL consume the correct number of clock cycles for each instruction as specified in the Motorola 6809 datasheet
6. THE CPU_Core SHALL operate as an externally clocked device (6809E variant), accepting clock signals from the Master_Clock rather than generating them internally
7. WHEN an IRQ interrupt is asserted and the I flag in CC is clear, THE CPU_Core SHALL push the entire machine state onto the S stack and vector through address 0xFFF8-0xFFF9
8. WHEN a FIRQ interrupt is asserted and the F flag in CC is clear, THE CPU_Core SHALL push only CC and PC onto the S stack and vector through address 0xFFF6-0xFFF7
9. WHEN an NMI interrupt occurs, THE CPU_Core SHALL push the entire machine state onto the S stack and vector through address 0xFFFC-0xFFFD
10. WHEN a SWI, SWI2, or SWI3 instruction is executed, THE CPU_Core SHALL push the appropriate state and vector through the corresponding vector address
11. THE CPU_Core SHALL implement the SYNC and CWAI instructions with correct halt-until-interrupt behavior
12. FOR ALL valid register values, executing an instruction and then its inverse (where applicable) SHALL restore the original register state (round-trip property)
13. FOR ALL valid operand pairs, the result of an ADD instruction followed by a SUB instruction with the same operand SHALL restore the original accumulator value (round-trip property)

### Requirement 4: MO5 Memory Map

**User Story:** As a developer, I want an accurate MO5 memory map implementation replacing the Videopac memory layout, so that software can access Video RAM, user RAM, ROM, and I/O at the correct addresses.

#### Acceptance Criteria

1. THE Memory_System SHALL map 64KB of address space according to the MO5 memory layout: Video RAM pixel data at 0x0000-0x1FFF (8KB), Video RAM color attributes at 0x2000-0x3FFF (8KB), user RAM at 0x4000-0x9FFF (24KB, with 0x6000-0x9FFF switchable to cartridge), I/O space at 0xA000-0xA7FF, reserved space at 0xA800-0xBFFF, BASIC ROM at 0xC000-0xEFFF (12KB), and Monitor ROM at 0xF000-0xFFFF (4KB)
2. THE Memory_System SHALL map PIA registers within the I/O space at 0xA000-0xA7FF according to MO5 hardware specifications
3. THE Memory_System SHALL map Gate_Array control registers within the I/O space at 0xA000-0xA7FF
4. THE Memory_System SHALL provide 48KB of total RAM: 16KB Video_RAM (0x0000-0x3FFF) and 32KB user RAM (0x4000-0x9FFF)
5. THE Memory_System SHALL load the MO5_BASIC ROM image into the address range 0xC000-0xEFFF (12KB)
6. THE Memory_System SHALL load the Monitor_ROM image into the address range 0xF000-0xFFFF (4KB), including the reset and interrupt vector table at 0xFFF0-0xFFFF
7. WHEN a cartridge is inserted, THE Memory_System SHALL map the cartridge ROM into the range 0x6000-0x9FFF, replacing user RAM in that region
8. IF a read targets an unmapped or reserved address in the range 0xA800-0xBFFF, THEN THE Memory_System SHALL return 0xFF
9. IF a write targets a ROM address in the range 0xC000-0xFFFF, THEN THE Memory_System SHALL ignore the write
10. FOR ALL addresses in the user RAM range 0x4000-0x5FFF and all byte values, writing a value and reading it back SHALL return the written value (round-trip property)
11. FOR ALL addresses in the Video_RAM range 0x0000-0x3FFF and all byte values, writing a value and reading it back SHALL return the written value (round-trip property)

### Requirement 5: PIA Emulation (MC6821) for MO5

**User Story:** As a developer, I want accurate PIA emulation mapped to MO5-specific I/O addresses, so that keyboard scanning, cassette control, light pen input, and interrupt routing work correctly.

#### Acceptance Criteria

1. THE PIA SHALL implement the MC6821 with two 8-bit data registers (DRA/DRB), two data direction registers (DDRA/DDRB), and two control registers (CRA/CRB), mapped within the MO5 I/O space at 0xA000-0xA7FF
2. WHEN bit 2 of a PIA control register is clear, THE PIA SHALL route reads and writes of the corresponding data address to the data direction register instead of the data register
3. WHEN bit 2 of a PIA control register is set, THE PIA SHALL route reads and writes of the corresponding data address to the data register
4. THE PIA SHALL mask data register reads through the data direction register, returning input pin states for bits configured as inputs and latched output values for bits configured as outputs
5. WHEN the PIA receives a vertical sync signal from the Gate_Array, THE PIA SHALL set the appropriate interrupt flag and assert IRQ to the CPU_Core if the corresponding interrupt enable bit is set
6. THE PIA SHALL provide keyboard column strobe output and row input lines for the Input_System to scan the MO5 chiclet keyboard matrix
7. THE PIA SHALL provide cassette data input and output lines for the Cassette_Interface
8. THE PIA SHALL provide light pen strobe input for the Light_Pen interface
9. WHEN the control register is read, THE PIA SHALL return the interrupt flags in bits 6 and 7 and clear them upon read
10. FOR ALL PIA data direction register values, reading the data register SHALL return (output_latch AND DDR) OR (input_pins AND NOT DDR) (invariant property)

### Requirement 6: Video Gate Array (EFGJ03L) Rendering

**User Story:** As a developer, I want accurate EFGJ03L gate array video rendering with the forme/fond color system replacing the Videopac 8245 VDC, so that MO5 graphics display correctly.

#### Acceptance Criteria

1. THE Gate_Array SHALL render a 320x200 pixel bitmap display by reading 8KB of pixel data from Video_RAM at 0x0000-0x1FFF
2. THE Gate_Array SHALL read 8KB of color attribute data from Video_RAM at 0x2000-0x3FFF, where each attribute byte defines a foreground color (forme) and background color (fond) for the corresponding 8-pixel block
3. THE Gate_Array SHALL interpret each attribute byte as: high nibble = foreground color index (forme, 0-15), low nibble = background color index (fond, 0-15)
4. THE Gate_Array SHALL render each pixel using the foreground color if the corresponding bit in the pixel data byte is 1, and the background color if the bit is 0
5. THE Gate_Array SHALL support the MO5 fixed palette of 16 colors as defined by the hardware
6. THE Gate_Array SHALL output pixel data to the Framebuffer as 32-bit RGBA values using the MO5 16-color palette
7. THE Gate_Array SHALL generate vertical sync and horizontal sync timing signals for interrupt generation via the PIA
8. THE Gate_Array SHALL render one complete frame of 320x200 pixels per display refresh cycle
9. WHEN a byte in Video_RAM pixel data (0x0000-0x1FFF) is modified, THE Gate_Array SHALL reflect the change in the Framebuffer on the next frame render
10. WHEN a byte in Video_RAM color attributes (0x2000-0x3FFF) is modified, THE Gate_Array SHALL reflect the color change in the Framebuffer on the next frame render

### Requirement 7: Audio System (1-Bit Buzzer)

**User Story:** As a developer, I want 1-bit buzzer audio emulation replacing the Videopac shift register audio, so that MO5 sound output plays correctly.

#### Acceptance Criteria

1. THE Audio_System SHALL emulate the MO5 1-bit buzzer by reading a single output bit toggled through the PIA
2. WHEN the buzzer bit is toggled, THE Audio_System SHALL produce a square wave output at the corresponding frequency
3. THE Audio_System SHALL resample the 1-bit buzzer output to the host sample rate (typically 44100Hz or 48000Hz) using appropriate filtering
4. THE Audio_System SHALL provide audio samples to the Frontend via a ring buffer to decouple emulation timing from audio callback timing
5. IF the audio ring buffer underflows, THEN THE Audio_System SHALL output silence to prevent audio artifacts

### Requirement 8: Keyboard Input (Chiclet Keyboard Matrix)

**User Story:** As a developer, I want MO5 chiclet keyboard matrix emulation adapted from the Videopac input handling, so that users can type and interact with MO5 software.

#### Acceptance Criteria

1. THE Input_System SHALL emulate the MO5 chiclet keyboard as a matrix scanned through the PIA, replacing the Videopac joystick input model
2. WHEN the PIA outputs a column strobe value, THE Input_System SHALL return the corresponding row state reflecting which keys in that column are pressed
3. THE Input_System SHALL map host keyboard keys to the MO5 keyboard matrix positions, including special keys (SHIFT, ENTER, STOP, CNT, ACC, and cursor keys)
4. THE Input_System SHALL support simultaneous key presses by correctly reporting multiple active rows when multiple keys in the same column are held
5. IF no key is pressed in a scanned column, THEN THE Input_System SHALL return all row bits in the inactive state

### Requirement 9: Light Pen Emulation (Crayon Optique)

**User Story:** As a developer, I want light pen emulation, so that MO5 software that uses the crayon optique can be used with a mouse.

#### Acceptance Criteria

1. THE Light_Pen SHALL translate host mouse position within the emulator window to the corresponding MO5 screen coordinates (0-319 horizontal, 0-199 vertical)
2. WHEN the mouse position corresponds to a valid screen location, THE Light_Pen SHALL generate a light pen strobe signal to the PIA at the correct time relative to the video scan position
3. THE Light_Pen SHALL provide the detected screen position to MO5 software through the PIA registers as the hardware light pen interface specifies
4. WHEN the host mouse button is clicked, THE Light_Pen SHALL signal a light pen activation event to the PIA
5. IF the mouse is outside the emulated display area, THEN THE Light_Pen SHALL report no light pen detection

### Requirement 10: Cassette Interface (K7 Format)

**User Story:** As a developer, I want cassette tape emulation using the K7 file format, so that users can load and save MO5 programs.

#### Acceptance Criteria

1. THE Cassette_Interface SHALL read K7 format file images containing Thomson MO5 cassette data
2. WHEN MO5 BASIC initiates a cassette read (LOAD command), THE Cassette_Interface SHALL provide data bytes from the loaded K7 file through the PIA cassette data input line
3. WHEN MO5 BASIC initiates a cassette write (SAVE command), THE Cassette_Interface SHALL capture the output data stream from the PIA cassette data output line and store it in K7 format
4. THE Cassette_Interface SHALL parse K7 files containing leader sequences, sync bytes, and data blocks with appropriate header and checksum fields as defined by the Thomson cassette format
5. THE Cassette_Interface SHALL serialize captured cassette output data into valid K7 format files
6. FOR ALL valid K7 file data, writing the data via SAVE and reading it back via LOAD SHALL produce identical byte sequences (round-trip property)
7. FOR ALL valid K7 files, parsing the file into an internal representation and serializing it back SHALL produce a byte-identical K7 file (round-trip property)

### Requirement 11: Master Clock and Timing

**User Story:** As a developer, I want accurate timing for the externally clocked 6809E, so that the emulator runs at the correct speed and components stay synchronized.

#### Acceptance Criteria

1. THE Master_Clock SHALL clock the CPU_Core at 1 MHz as specified for the MO5 hardware
2. THE Master_Clock SHALL provide the external clock signal (E and Q phases) to the 6809E CPU_Core, as the 6809E does not generate its own clock
3. THE Master_Clock SHALL synchronize the Gate_Array video timing with the CPU_Core clock to maintain correct display refresh
4. THE Master_Clock SHALL generate one complete video frame per display refresh cycle (50Hz for PAL-based MO5 timing)
5. FOR ALL frame durations, the number of CPU cycles per frame SHALL remain constant within a tolerance of one cycle (invariant property)

### Requirement 12: Emulator Core Integration

**User Story:** As a developer, I want a unified emulator core that coordinates all components, adapted from the Videopac emulator core architecture, so that the MO5 boots to BASIC 1.0 and runs software.

#### Acceptance Criteria

1. THE Emulator SHALL initialize all components (CPU_Core, Gate_Array, Memory_System, Audio_System, Input_System, Light_Pen, PIA, Cassette_Interface) and connect them via their respective interfaces during startup
2. THE Emulator SHALL execute a run_frame() method that advances all components by exactly one video frame worth of CPU cycles
3. WHEN reset is triggered, THE Emulator SHALL reset all components to their power-on state and begin execution from the reset vector at 0xFFFE-0xFFFF as stored in the Monitor_ROM
4. THE Emulator SHALL load the MO5_BASIC ROM and Monitor_ROM images and boot to the BASIC 1.0 prompt when no cartridge is inserted
5. THE Emulator SHALL provide save_state and load_state methods that serialize and deserialize the complete state of all components using the adapted Save_State_Framework
6. FOR ALL valid emulator states, saving state and loading it back SHALL produce an emulator that behaves identically to the original from that point forward (round-trip property)
7. FOR ALL valid emulator states, saving state, serializing to bytes, and deserializing back SHALL produce an identical state object (round-trip property)

### Requirement 13: SDL2 Frontend

**User Story:** As a developer, I want to adapt the existing SDL2 frontend from the Videopac codebase, so that I can see video output, hear audio, and provide input to the MO5 emulator.

#### Acceptance Criteria

1. THE Frontend SHALL be adapted from the Videopac_Codebase Frontend (src/frontend_sdl.cpp, include/frontend_sdl.h) to display the MO5 Gate_Array Framebuffer scaled to a configurable integer multiple (default 3x: 960x600)
2. THE Frontend SHALL present the Framebuffer to the display using SDL2 hardware-accelerated rendering with VSync enabled
3. THE Frontend SHALL capture SDL2 keyboard events and translate them to MO5 keyboard matrix state changes via the Input_System, replacing the Videopac joystick event handling
4. THE Frontend SHALL capture SDL2 mouse events and translate them to Light_Pen coordinates and activation state
5. THE Frontend SHALL open an SDL2 audio device at 48000Hz and feed samples from the Audio_System ring buffer
6. WHEN the user presses the Escape key, THE Frontend SHALL toggle the emulator pause state
7. WHEN the user closes the window, THE Frontend SHALL cleanly shut down the Emulator and release all SDL2 resources
8. IF the Emulator runs faster than real-time, THEN THE Frontend SHALL throttle execution to maintain the correct frame rate

### Requirement 14: Libretro Core

**User Story:** As a developer, I want to adapt the existing Libretro core from the Videopac codebase, so that Crayon can run as a core in RetroArch and other libretro frontends.

#### Acceptance Criteria

1. THE Libretro_Core SHALL be adapted from the Videopac_Codebase Libretro implementation (src/libretro.cpp, include/libretro.h) by swapping the Videopac EmulatorCore internals with the MO5 Emulator
2. THE Libretro_Core SHALL implement the standard libretro API functions (retro_init, retro_deinit, retro_run, retro_load_game, retro_unload_game, retro_serialize, retro_unserialize, retro_get_system_info, retro_get_system_av_info)
3. THE Libretro_Core SHALL report MO5-specific system information: core name "crayon", supported extensions "k7,rom", and MO5 display geometry (320x200)
4. THE Libretro_Core SHALL provide video frames from the Gate_Array Framebuffer to the libretro frontend in XRGB8888 pixel format
5. THE Libretro_Core SHALL provide audio samples from the Audio_System to the libretro frontend at the reported sample rate
6. THE Libretro_Core SHALL translate libretro input callbacks to MO5 keyboard matrix state and Light_Pen coordinates via the Input_System
7. THE Libretro_Core SHALL support save state serialization and deserialization through the libretro retro_serialize and retro_unserialize API using the Save_State_Framework
8. THE Libretro_Core SHALL produce a shared library (crayon_libretro.so on Linux, crayon_libretro.dll on Windows, crayon_libretro.dylib on macOS) loadable by RetroArch

### Requirement 15: ImGui Debugger UI

**User Story:** As a developer, I want to adapt the existing ImGui debugger UI from the Videopac codebase, so that I can inspect and debug MO5 emulation interactively.

#### Acceptance Criteria

1. THE Debugger_UI SHALL be adapted from the Videopac_Codebase debugger (src/debugger_ui.cpp, include/debugger_ui.h, include/debugger.h) to display 6809E CPU state instead of 8048 CPU state
2. THE Debugger_UI SHALL display all 6809E registers (A, B, D, X, Y, U, S, PC, DP, CC) with real-time updates during execution
3. THE Debugger_UI SHALL display the CC register flags (E, F, H, I, N, Z, V, C) individually with labeled indicators
4. THE Debugger_UI SHALL provide a memory viewer that displays the MO5 64KB address space with region labels (Video RAM, User RAM, I/O, ROM)
5. THE Debugger_UI SHALL provide a disassembly view that decodes 6809 instructions at and around the current PC, including the 0x10 and 0x11 prefix pages
6. THE Debugger_UI SHALL support setting and clearing execution breakpoints at specified addresses
7. THE Debugger_UI SHALL support single-stepping the CPU_Core by one instruction
8. THE Debugger_UI SHALL provide step-over and step-out functionality for subroutine calls (JSR/BSR)
9. THE Debugger_UI SHALL display PIA register state (DRA, DRB, DDRA, DDRB, CRA, CRB) and Gate_Array state for peripheral debugging
10. WHEN the user toggles the debugger, THE Debugger_UI SHALL pause emulation and display the current CPU and memory state

### Requirement 16: CI/CD Pipeline

**User Story:** As a developer, I want to adapt the existing GitHub Actions CI/CD pipeline from the Videopac codebase, so that Crayon is continuously built, tested, and released.

#### Acceptance Criteria

1. THE CI_CD_Pipeline SHALL be adapted from the Videopac_Codebase GitHub Actions workflows (.github/ directory) to build and test the Crayon project
2. THE CI_CD_Pipeline SHALL build the Crayon executable and test suite on Linux (GCC), macOS (Clang), and Windows (MSVC) for each push and pull request
3. THE CI_CD_Pipeline SHALL run the full Test_Harness (unit tests and property-based tests) and report failures
4. THE CI_CD_Pipeline SHALL build the Libretro_Core shared library as a build artifact
5. IF any test fails during a CI run, THEN THE CI_CD_Pipeline SHALL mark the build as failed and report the failing test names

### Requirement 17: Property-Based Testing for CPU Instructions

**User Story:** As a developer, I want property-based tests for CPU instruction correctness, so that I can catch subtle instruction encoding and flag-setting bugs.

#### Acceptance Criteria

1. THE Test_Harness SHALL use RapidCheck to generate random register states and operand values for CPU instruction tests
2. FOR ALL 8-bit operand pairs, the result of ADDA followed by SUBA with the same operand SHALL restore the original A register value (round-trip property)
3. FOR ALL 16-bit operand pairs, the result of ADDD followed by SUBD with the same operand SHALL restore the original D register value (round-trip property)
4. FOR ALL valid register sets, a PSHS of all registers followed by PULS of all registers SHALL restore the original register state (round-trip property)
5. FOR ALL 8-bit values, NEG applied twice SHALL return the original value except for 0x80 (round-trip property with known exception)
6. FOR ALL 8-bit values, COM applied twice SHALL return the original value (round-trip property)
7. FOR ALL condition code states after an arithmetic operation, the N flag SHALL equal bit 7 of the result and the Z flag SHALL be set if and only if the result is zero (invariant property)
8. FOR ALL pairs of values A and B, CMPA with B SHALL set the same flags as SUBA with B except CMPA SHALL preserve the original A value (metamorphic property)

### Requirement 18: Property-Based Testing for Memory Mapping

**User Story:** As a developer, I want property-based tests for memory mapping correctness, so that I can verify MO5 address decoding and region behavior.

#### Acceptance Criteria

1. FOR ALL addresses in user RAM range 0x4000-0x5FFF and all byte values, writing a value and reading it back SHALL return the written value (round-trip property)
2. FOR ALL addresses in Video_RAM range 0x0000-0x3FFF and all byte values, writing a value and reading it back SHALL return the written value (round-trip property)
3. FOR ALL addresses in ROM range 0xC000-0xFFFF, writes SHALL have no effect and reads SHALL return the original ROM content (invariant property)
4. FOR ALL PIA data direction register values, reading the data register SHALL return (output_latch AND DDR) OR (input_pins AND NOT DDR) (invariant property)
5. FOR ALL valid address ranges, reading from a memory-mapped I/O address in 0xA000-0xA7FF SHALL route to the correct peripheral component (invariant property)
6. FOR ALL addresses in the reserved range 0xA800-0xBFFF, reads SHALL return 0xFF and writes SHALL have no observable effect (invariant property)
