# Crayon — Thomson MO5 Emulator

A Thomson MO5 home computer emulator written in C++17. Runs as a standalone SDL2 app, a headless CLI tool, or a libretro core.

The Thomson MO5 (1984) was a popular French home computer with a Motorola 6809E CPU, 48 KB RAM, 320×200 16-color graphics, and cassette-based storage.

## Features

- Full Motorola 6809E CPU emulation
- 320×200 bitmap video with 16-color palette
- MO5 keyboard input (AZERTY layout, scancode-based)
- Light pen emulation via mouse
- K7 cassette loading (fast and slow modes)
- 1-bit buzzer audio
- Save states with thumbnails
- ImGui debugger with disassembly, breakpoints, memory viewer
- In-game menu system, file browser, OSD
- Auto-type keystroke injection (both SDL and headless)
- Libretro core for RetroArch

## Building

Requires CMake 3.15+, a C++17 compiler, and SDL2.

```bash
# Configure (MinGW example)
cmake --preset dev-mingw

# Build
cmake --build --preset dev-mingw

# Run tests
ctest --preset dev-mingw
```

See `CMakePresets.json` and `CMakeUserPresets.json.example` for available presets. CI builds on Ubuntu (GCC), macOS (Clang), Windows (MSVC + MinGW), and Android (NDK).

## ROMs

You need the MO5 BASIC and Monitor ROMs to run the emulator. Place them in a `roms/` directory:

- `roms/basic5.rom` — 12 KB BASIC 1.0 ROM
- `roms/mo5.rom` — 4 KB Monitor ROM

These are copyrighted and not included. They can be dumped from real hardware or found in various Thomson preservation archives.

## Running

The `run_emulator.py` launcher script handles platform detection and common options:

```bash
# Boot to BASIC prompt (SDL window)
python run_emulator.py

# With a cassette game (fast load + auto-type)
python run_emulator.py sdl --cassette "roms/game.k7" --k7-mode fast \
  --type 'LOAD""\n\w200RUN\n' --type-delay 60

# Same thing using a type file (avoids shell quoting issues)
python run_emulator.py sdl --cassette "roms/game.k7" --k7-mode fast \
  --type-file load_run.txt --type-delay 60

# Headless mode (500 frames, auto-screenshot)
python run_emulator.py hl --frames 500 --screenshot screenshots/boot.png

# With debugger
python run_emulator.py sdl --debugger
```

Or run the executable directly:

```bash
build/dev-mingw/crayon.exe --basic roms/basic5.rom --monitor roms/mo5.rom
```

### Keyboard shortcuts

| Key | Action |
|-----|--------|
| F1 | Toggle menu |
| F5 | Toggle debugger |
| F6 | Play/stop cassette (or load K7 if none loaded) |
| F7 | Reset |
| F8 | Pause/resume |
| F9 | Save state |
| F10 | Load state |
| F11 | Screenshot |
| F12 | Toggle FPS overlay |

### Auto-type

Inject keystrokes from the command line with `--type` or `--type-file`. Works in both SDL and headless modes.

- `\n` — ENTER key (only needed with `--type`; type files have real newlines)
- `\wNNN` — wait NNN frames before continuing

Example type file (`load_run.txt`):
```
LOAD""
\w200RUN
```

With `--type-file`, newlines in the file are interpreted as ENTER keypresses directly — no `\n` escape needed.

### Cassette loading

- `--k7-mode fast` (default) — direct data injection, loads in a few frames
- `--k7-mode slow` — real-time 1200 baud simulation

## Project structure

```
include/          C++ headers (emulator core, frontends, UI)
src/              C++ source files
docs/             Hardware reference documentation
debug/            Trace files, test scripts
roms/             ROM files (not tracked)
third_party/      Vendored libraries (stb, miniz)
.github/          CI workflows
```

## Documentation

- `docs/mo5-hardware.md` — detailed MO5 hardware reference with ROM disassembly notes

## License

MIT
