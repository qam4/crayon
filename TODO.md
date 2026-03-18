# Crayon MO5 Emulator — TODO List

Audit date: 2026-03-18. Covers all specs, source TODOs, and stubs.

---

## Joystick Input (done)

- [x] Add joystick event handling in `process_input()` (`SDL_JOYBUTTONDOWN/UP`, `SDL_JOYAXISMOTION`)
- [x] Implement `init_joysticks()`, `handle_joystick_button_event()`, `handle_joystick_axis_event()` in `frontend_sdl.cpp`
- [x] Add default joystick mappings (D-pad→arrows, button→SPACE/ENTER) in `InputMapper::init_default_mappings()`
- [x] Add joystick tab/section to `InputMapper::render_mapping_ui()`
- [x] Implement `InputMapper::load_from_config()` — joystick INI persistence
- [x] Implement `InputMapper::save_to_config()` — joystick INI persistence
- [x] Init `SDL_INIT_JOYSTICK` in `init_video()`
- [x] Joystick hotplug support (`SDL_JOYDEVICEADDED/REMOVED`)
- [x] Joystick remapping UI capture (`process_joystick_mapping_input()`)

## Cassette / K7 (spec: crayon-emulator task 11)

- [ ] **11.8** Slow loading mode (1200 baud audio simulation) — `read_data_bit()` exists but needs refinement to present bits from parsed K7 blocks with leader tones, sync bytes, and inter-block gaps
- [ ] **11.9** Load mode toggle in UI — menu item, Shift+F6 shortcut, status bar indicator, ConfigManager persistence
- [ ] **11.10** Test K7 loading with real games — fast and slow mode validation, document timing-sensitive loaders
- [ ] Headless `--autoload` flag for batch K7 compatibility testing — auto-type LOAD""/LOADM based on K7 type, detect success/failure, report result for compatibility database

## Bugfixes (spec: emulator-bugfixes-batch1)

- [ ] **9** Verify Bug 3: File browser responsive sizing at 640×400
- [ ] **10.1** Add `get/set_last_k7_path()`, `get/set_last_basic_rom_path()`, `get/set_last_monitor_rom_path()` to ConfigManager
- [ ] **10.2** Distinguish BASIC ROM vs Monitor ROM `FileType` in file browser
- [ ] **10.3** Save file paths on selection in `frontend_sdl.cpp`
- [ ] **10.4** Auto-load logic in `SDLFrontend::initialize()`
- [ ] **10.5–10.6** Verify config persistence tests pass
- [ ] **11** Final checkpoint — run all exploration + preservation tests

## Libretro Performance (spec: libretro-performance)

- [ ] **4.1** Inline MasterClock hot accessors into header (non-LTO benefit)
- [ ] **4.2–4.3** Property test + verify for MasterClock inlining
- [ ] **6.1–6.4** `run_frame()` fast path: hoist cassette checks before loop
- [ ] **7.1–7.6** Palette LUT for libretro framebuffer (eliminate `rgba_to_xrgb()` loop)
- [ ] **8.2–8.3** Benchmark regression gate + update profiling docs

## SDL Frontend — Source TODOs

- [ ] `process_audio()` — empty body, needs to fill SDL audio buffer from emulator (`frontend_sdl.cpp:300`)
- [ ] `save_screenshot()` — stub, needs stb_image_write or equivalent (`frontend_sdl.cpp:493`)
- [ ] `dump_framebuffer()` — stub (`frontend_sdl.cpp:497`)
- [ ] `save_state_manager_->render_ui("current_game")` — hardcoded string, should use actual game name (`frontend_sdl.cpp:214`)
- [ ] Status bar: show loaded K7 filename (not just play state)

## Debugger — Source TODOs

- [ ] `Debugger::dump_memory()` — returns "not yet implemented" (`debugger.cpp:84`)
- [ ] `Debugger::disassemble_at_pc()` — returns "not yet implemented" (`debugger.cpp:91`)
- [ ] `Debugger::evaluate_condition()` — condition expression parser not implemented (`debugger.cpp:118`)

## ZIP Handler

- [ ] `ZIPHandler` — `list_contents()` and `extract_file()` are stubs, need miniz or libzip integration (`zip_handler.cpp`)

## Optional / Property Tests (all specs, marked `*`)

These are marked optional in every spec. Listing for completeness — skip for MVP.

### crayon-emulator
- [ ] 3.11–3.20: CPU property tests (ADDA/SUBA round-trip, PSHS/PULS, NEG, COM, flags, CMPA, interrupts, cycles, disassembly)
- [ ] 5.7–5.15: Memory/PIA property tests (RAM round-trip, ROM write-ignore, reserved range, I/O routing, DDR/DR, masking, interrupt flag, vsync, cartridge)
- [ ] 7.4: Forme/fond rendering property test
- [ ] 9.5–9.7: Timing/save-state/reset-vector property tests
- [ ] 11.11–11.12: K7 parse/serialize and write/read round-trip
- [ ] 12.4: Keyboard matrix scanning property test
- [ ] 14.4: Light pen coordinate translation property test
- [ ] 15.7–15.8: Audio buzzer + libretro API compliance tests
- [ ] 17.12–17.26: UI component unit/property tests (ConfigManager, MenuSystem, FileBrowser, OSD, Dialogs, InputMapper, SaveStateUI, RecentFiles, ZIPHandler, TextRenderer)

### libretro-integration
- [ ] 1.3–1.6: VKB property/unit tests
- [ ] 3.3–3.4: Save state serialization property tests
- [ ] 4.3–4.5: File extension routing + BIOS discovery tests
- [ ] 6.3: Core options unit tests
- [ ] 7.4–7.6: Input mapping property/unit tests
- [ ] 9.3–9.4: Audio conversion + save state unit tests

### libretro-performance
- [ ] 4.2: MasterClock tick state determinism property test
- [ ] 6.3: run_frame fast path equivalence property test
- [ ] 7.6: Palette round-trip + GateArray palette mode property tests
