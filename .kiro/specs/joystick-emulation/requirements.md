# Requirements Document

## Introduction

The Thomson MO5 has a game extension PIA (MC6821) at `$A7CC–$A7CF` that provides two joystick ports. Port A carries direction inputs (up, down, left, right) for both joysticks, and Port B bits 6–7 carry the fire buttons. The Crayon emulator currently implements the game PIA for 6-bit DAC audio output but returns hardcoded values for joystick inputs (Port A = `0xFF`, Port B bits 6–7 = `0xC0`), meaning joystick-based games receive no input.

This feature wires the game PIA joystick ports to actual host input — RetroPad in the libretro frontend and keyboard/gamepad in the SDL frontend — so that joystick-driven MO5 games (e.g. "Top Chrono", "Aigle d'Or") respond to player input. A port-swap option is included because some MO5 games read joystick 2 instead of joystick 1.

## Glossary

- **Game_PIA**: The MC6821 Peripheral Interface Adapter at `$A7CC–$A7CF` on the MO5, used for joystick input and 6-bit DAC audio output. Also called the "music & games" PIA.
- **Port_A**: Game_PIA Port A (accent register 0). Bits 0–3 carry joystick 1 directions; bits 4–7 carry joystick 2 directions. Each direction bit is active low (0 = pressed, 1 = released).
- **Port_B**: Game_PIA Port B (register 2). Bits 0–5 are the 6-bit DAC output. Bit 6 is the joystick 1 fire button; bit 7 is the joystick 2 fire button. Fire bits are active low.
- **Joystick_State**: An abstraction holding the current pressed/released state of four directions (up, down, left, right) and one fire button for a single joystick port.
- **InputHandler**: The existing `crayon::InputHandler` class that manages MO5 keyboard state. Extended to also hold joystick state.
- **RetroPad**: The libretro standard gamepad abstraction, providing D-pad directions and face buttons.
- **SDL_Frontend**: The standalone SDL2-based frontend (`src/frontend_sdl.cpp`) that handles keyboard and SDL gamepad input.
- **Libretro_Frontend**: The libretro core frontend (`src/libretro.cpp`) that handles RetroPad input.
- **Core_Option**: A libretro core option exposed via `retro_set_environment` that the user can configure in the RetroArch menu.
- **Port_Swap**: A configuration option that exchanges the mapping of host controller 1 and host controller 2 to MO5 joystick port 1 and joystick port 2.
- **MemorySystem**: The existing `crayon::MemorySystem` class that handles all MO5 memory-mapped I/O including the Game_PIA.

## Requirements

### Requirement 1: Joystick State Storage

**User Story:** As an emulator developer, I want a joystick state representation in the input layer, so that both frontends can write joystick state and the Game_PIA can read it.

#### Acceptance Criteria

1. THE InputHandler SHALL store Joystick_State for two joystick ports (port 1 and port 2).
2. THE InputHandler SHALL provide methods to set individual direction and fire button states for a specified joystick port.
3. THE InputHandler SHALL provide a method to read the combined joystick direction byte for Port_A (bits 0–3 = port 1, bits 4–7 = port 2, active low).
4. THE InputHandler SHALL provide a method to read the joystick fire button bits for Port_B (bit 6 = port 1 fire, bit 7 = port 2 fire, active low).
5. WHEN the emulator resets, THE InputHandler SHALL set all joystick directions and fire buttons to the released state.

### Requirement 2: Game PIA Joystick Wiring

**User Story:** As a player, I want the Game_PIA to reflect actual joystick input, so that joystick-based MO5 games respond to my controller.

#### Acceptance Criteria

1. WHEN a game reads Game_PIA Port_A, THE MemorySystem SHALL return direction bits from the InputHandler Joystick_State instead of the hardcoded value `0xFF`.
2. WHEN a game reads Game_PIA Port_B, THE MemorySystem SHALL return fire button bits 6–7 from the InputHandler Joystick_State instead of the hardcoded value `0xC0`, while preserving bits 0–5 (DAC output) unchanged.
3. THE MemorySystem SHALL respect the Game_PIA data direction registers: only bits configured as inputs in the DDR SHALL read from the Joystick_State; bits configured as outputs SHALL read back the output register value.
4. THE MemorySystem SHALL use active-low encoding for all joystick bits (0 = pressed, 1 = released).

### Requirement 3: Libretro RetroPad Mapping

**User Story:** As a RetroArch player, I want my RetroPad D-pad and buttons to control the MO5 joystick, so that I can play joystick games in RetroArch.

#### Acceptance Criteria

1. THE Libretro_Frontend SHALL map RetroPad D-pad Up, Down, Left, Right on controller port 0 to MO5 joystick port 1 directions.
2. THE Libretro_Frontend SHALL map RetroPad button B on controller port 0 to MO5 joystick port 1 fire button.
3. THE Libretro_Frontend SHALL map RetroPad D-pad and button B on controller port 1 to MO5 joystick port 2 directions and fire button.
4. WHEN the virtual keyboard is visible, THE Libretro_Frontend SHALL continue to update joystick state from the RetroPad, independent of VKB navigation.
5. THE Libretro_Frontend SHALL sample RetroPad joystick input once per frame during `retro_run`.

### Requirement 4: SDL Frontend Joystick Mapping

**User Story:** As a standalone player, I want to use my keyboard or USB gamepad to control the MO5 joystick in the SDL frontend, so that I can play joystick games without RetroArch.

#### Acceptance Criteria

1. THE SDL_Frontend SHALL map SDL gamepad D-pad and a configurable fire button to MO5 joystick port 1 directions and fire button.
2. THE SDL_Frontend SHALL map a second SDL gamepad (if connected) to MO5 joystick port 2.
3. THE SDL_Frontend SHALL provide a keyboard fallback mapping for joystick port 1 using arrow keys for directions and a configurable key for fire.
4. WHEN both a keyboard joystick mapping and an SDL gamepad provide input for the same joystick port, THE SDL_Frontend SHALL combine the inputs using logical OR (either source can activate a direction or fire).

### Requirement 5: Joystick Port Swap Option

**User Story:** As a player, I want to swap joystick ports, so that I can play MO5 games that read joystick port 2 instead of port 1 (e.g. some two-player games used solo on port 2).

#### Acceptance Criteria

1. THE Libretro_Frontend SHALL expose a Core_Option named `crayon_joystick_port_swap` with values "disabled" (default) and "enabled".
2. WHEN `crayon_joystick_port_swap` is set to "enabled", THE Libretro_Frontend SHALL route RetroPad port 0 input to MO5 joystick port 2 and RetroPad port 1 input to MO5 joystick port 1.
3. WHEN `crayon_joystick_port_swap` is set to "disabled", THE Libretro_Frontend SHALL route RetroPad port 0 to MO5 joystick port 1 and RetroPad port 1 to MO5 joystick port 2.
4. THE SDL_Frontend SHALL provide an equivalent port swap toggle accessible via a menu option or keyboard shortcut.
5. WHEN the port swap setting changes at runtime, THE InputHandler SHALL clear all joystick state to the released state before applying the new mapping.

### Requirement 6: DAC Audio Preservation

**User Story:** As a player, I want joystick support to coexist with the existing 6-bit DAC audio, so that games with both music and joystick input work correctly.

#### Acceptance Criteria

1. THE MemorySystem SHALL continue to route Game_PIA Port_B bits 0–5 writes to the AudioSystem DAC, unchanged by joystick emulation.
2. WHEN a game writes to Game_PIA Port_B, THE MemorySystem SHALL update only the DAC output bits (0–5) and leave the joystick fire button bits (6–7) as read-only inputs.
3. THE MemorySystem SHALL allow simultaneous DAC audio output and joystick fire button input on Port_B without interference.

### Requirement 7: Save State Compatibility

**User Story:** As a player, I want joystick state to be included in save states, so that loading a save state restores the correct joystick input context.

#### Acceptance Criteria

1. THE InputHandler SHALL include Joystick_State for both ports in the serialized save state data.
2. WHEN a save state is loaded, THE InputHandler SHALL restore the Joystick_State from the save state data.
3. IF a save state created before joystick support is loaded, THEN THE InputHandler SHALL initialize Joystick_State to all-released (backward compatibility).
