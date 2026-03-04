---
inclusion: manual
---

# Emulator Debugging Methodology

## Principles

1. **The ROM is ground truth.** It defines what the hardware does. Don't assume wiring from datasheets — verify against the actual ROM code.
2. **Don't guess at the code level.** If a fix feels like a workaround, the root cause is probably elsewhere.
3. **Use existing tools first.** Enhance them if needed — don't create throwaway scripts.
4. **Trace before you touch.** Always capture a trace of the broken behavior before attempting a fix.

## Available Tools

### run_emulator.py

Cross-platform launcher that finds the built executable and passes arguments through.

```bash
# SDL mode (default) — visual debugging
python run_emulator.py sdl --debugger
python run_emulator.py sdl --cassette "roms/game.k7" --debugger

# Headless mode — automated capture
python run_emulator.py headless --frames 500 --trace debug/trace.txt
python run_emulator.py headless --frames 300 --type 'LOAD"",,R\n' --trace debug/load_trace.txt
python run_emulator.py headless --cassette "roms/game.k7" --type 'LOADM"",,R\n' --type-delay 60 --frames 1000 --trace debug/game_trace.txt
```

Key options:
- `--trace <path>` — dump per-frame CPU/PIA/cassette state to file
- `--type <text>` — inject keystrokes (`\n` = ENTER), simulates typing on the MO5 keyboard
- `--type-file <path>` — read keystrokes from file (avoids shell quoting)
- `--type-delay <n>` — frames to wait before typing (default 60, lets BASIC boot first)
- `--frames <n>` — run exactly N frames then exit
- `--screenshot <path>` — save PNG at end of run (auto-generated in headless mode)

### crayon --headless --trace (direct)

Same as above but calling the executable directly. Trace output includes per-frame:
PC, CC flags (F/I bits), FIRQ vector, frame counter, cursor enable, PIA irqb1 flag, CRB,
and cassette state (playing/stopped, position, start/current cycle).

First 60 frames logged individually, then every 50th frame.

### crayon --debugger (interactive)

Interactive debugger with ImGui UI. Available in SDL mode. Toggle with F5.

Features: breakpoints, single-step, memory dump, disassembly, register view,
CPU/Video Color/Video Shape memory tabs, instruction trace logging.

### disasm_roms (standalone tool)

Generic 6809 ROM disassembler. Built alongside the emulator.

```bash
# Build location: build/dev-mingw/disasm_roms.exe
build/dev-mingw/disasm_roms roms/mo5.rom F000 debug/monitor_rom.asm
build/dev-mingw/disasm_roms roms/basic5.rom C000 debug/basic_rom.asm
```

Output includes interrupt vector table, known-address labels, and full disassembly.
Existing disassemblies: `debug/monitor_rom.asm`, `debug/basic_rom.asm`.

### boot_diag (standalone tool)

Boots the emulator for 300 frames and renders an ASCII-art framebuffer + PNG screenshot.
Useful for quick visual sanity checks without SDL.

```bash
build/dev-mingw/boot_diag
# Output: screenshots/boot_diag.png + ASCII framebuffer to stdout
```

### screenshots/ and debug/ directories

- `screenshots/` — PNG framebuffer captures (headless auto-saves here)
- `debug/` — trace logs, disassemblies, diagnostic output. All debug artifacts go here.

## Debugging Workflow

### General: Something is broken

1. **Reproduce** — `python run_emulator.py headless --frames N --trace debug/issue_trace.txt`
2. **Screenshot** — check `screenshots/frame_N.png` to see what the screen looks like
3. **Read the trace** — look at PC, flags, frame counter progression
4. **Disassemble** — if the issue involves ROM behavior, check `debug/monitor_rom.asm` or `debug/basic_rom.asm`, or re-disassemble with `disasm_roms`
5. **Cross-reference** — find the ROM handler (interrupt vectors, SWI table, RAM vectors at $2058-$2076) and trace what it actually does
6. **Fix the emulator** — match behavior to what the ROM expects
7. **Verify** — re-run the trace, confirm state changes match expectations

### Game frozen / stuck in a loop

When a game appears frozen (screen static, no response to input):

1. **Capture a long trace with the game loaded:**
   ```bash
   python run_emulator.py headless --cassette "roms/game.k7" \
     --type 'LOADM"",,R\n' --type-delay 60 --frames 2000 \
     --trace debug/frozen_trace.txt
   ```

2. **Take screenshots at intervals** — run multiple times with different `--frames` values
   to see if the screen changes at all, or if it freezes at a specific point.

3. **Look at the trace for PC patterns** — if PC is cycling through a small range of
   addresses, the CPU is stuck in a tight loop. Identify the loop:
   - Is it a polling loop waiting for a hardware flag? (e.g., PIA bit, cassette bit)
   - Is it waiting for an interrupt that never fires?
   - Is it a SYNC or CWAI instruction waiting for a signal?

4. **Disassemble the loop** — use `disasm_roms` on the relevant ROM, or use the
   interactive debugger to disassemble around the stuck PC address.

5. **Trace back** — once you know what the loop is waiting for, trace back through
   the emulator code to find why that condition is never met. Common causes:
   - Interrupt not wired correctly (wrong line, wrong priority)
   - Hardware register not returning expected value
   - Timing mismatch (cycle count off, baud rate wrong)
   - Memory mapping error (read/write going to wrong address)

6. **Fix and verify** — apply the fix, re-run the same trace, confirm the loop exits.

### Cassette loading issues

1. **Trace with cassette and LOAD command:**
   ```bash
   python run_emulator.py headless --cassette "roms/game.k7" \
     --type 'LOAD""\n' --type-delay 60 --frames 3000 \
     --trace debug/k7_load_trace.txt
   ```
   For machine-language programs use `LOADM"",,R\n` instead.

2. **Check cassette state in trace** — look for `K7:playing pos=X/Y start_cy=Z cur_cy=W`.
   If position never advances, the ROM isn't reading the cassette port.
   If it advances but loading fails, bit timing or data format may be wrong.

3. **Check the ROM's cassette read routine** — in `debug/monitor_rom.asm`, find the
   code at the cassette read vector. It typically polls $A7C0 bit 7 for data bits.

4. **Compare with known-good behavior** — reference dcmoto or hardware documentation
   for expected timing and protocol.

### Interrupt / timing issues

1. **Check interrupt vectors** — disassemble the ROM and verify which vectors point where:
   - $FFF6 = FIRQ, $FFF8 = IRQ, $FFFA = SWI, $FFFC = NMI, $FFFE = RESET
   - RAM vectors at $2058-$2076 may redirect interrupts

2. **Trace CC flags** — the F and I bits in CC control interrupt masking.
   If F=1, FIRQ is masked. If I=1, IRQ is masked. Check the trace for these.

3. **Verify PIA wiring** — PIA IRQA and IRQB should map to the correct CPU interrupt
   lines. On MO5: both PIA interrupts → CPU IRQ (not FIRQ).

## MO5 Quick Reference

Key memory-mapped I/O:
- `$A7C0` — Gate array register (video mode, cassette data bit 7)
- `$A7C1` — PIA Port B (keyboard column)
- `$A7C2` — PIA CRA (control register A)
- `$A7C3` — PIA CRB (control register B, vsync interrupt)

Key RAM locations:
- `$2031` — Frame counter (incremented by IRQ handler each vsync)
- `$2063` — Cursor enable flag
- `$2058-$2076` — RAM interrupt redirect vectors

Interrupt vectors (ROM):
- `$FFFE` = RESET → boot sequence
- `$FFF8` = IRQ → vsync handler (frame counter, cursor blink)
- `$FFF6` = FIRQ → typically RTI stub on MO5 (unused)
