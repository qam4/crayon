---
inclusion: manual
---

# Emulator Debugging Methodology

## Principles

1. **The ROM is ground truth.** It defines what the hardware does. Don't assume wiring from datasheets — verify against the actual ROM code.
2. **Don't guess at the code level.** If a fix feels like a workaround, the root cause is probably elsewhere.
3. **Enhance existing tools, don't create new ones.** Check what's available below before writing throwaway scripts.

## Available Tools

### crayon --headless --trace

Boot trace that dumps per-frame CPU/PIA/memory state. Use for verifying interrupt handling, boot sequence, cursor behavior, etc.

```
crayon --headless --frames 500 --trace debug/trace.txt
```

Output includes PC, CC flags, FIRQ vector, frame counter, cursor enable, PIA irqb1 flag, and CRB per frame. First 60 frames are logged individually, then every 50th frame.

### crayon --debugger

Interactive debugger with breakpoints, step execution, memory dump, disassembly, and trace logging. Available in both SDL and headless modes.

Commands (in debugger console):
- `s` / `step` — single-step one instruction
- `c` / `continue` — resume execution
- `b <addr>` / `break` — set breakpoint
- `m <start> <end>` / `memory` — dump memory range
- `dis` / `disassemble` — disassemble around PC
- `t on|off` / `trace` — toggle instruction trace logging

### disasm_roms (standalone)

Generic 6809 ROM disassembler. Disassembles any ROM file with a base address.

```
disasm_roms <rom_file> <base_hex_addr> [output_file]
```

Examples:
```
disasm_roms roms/mo5.rom F000 debug/monitor_rom.asm
disasm_roms roms/basic5.rom C000 debug/basic_rom.asm
```

Output includes interrupt vector table, labeled addresses, and full disassembly. Existing disassemblies live in `debug/`.

## Debugging Workflow

1. **Reproduce** — use `--headless --frames N --trace` to capture state around the problem
2. **Disassemble** — if the issue involves ROM behavior, disassemble the relevant ROM with `disasm_roms`
3. **Cross-reference** — find the ROM handler for the feature (interrupt vectors, SWI table, RAM vectors at $2058-$2076) and trace what it actually does
4. **Fix the emulator** — match the emulator's behavior to what the ROM expects
5. **Verify** — re-run the trace and confirm the state changes match expectations
