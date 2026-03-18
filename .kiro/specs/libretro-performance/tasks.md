# Tasks: Libretro Performance Optimization

## Profiling Baseline

| Metric | Profile (-O2) | LTO (-O2 -flto) | Delta |
|--------|--------------|-----------------|-------|
| Cycles | 851.8M | 393.4M | -53.8% |
| Instructions | 3,110M | 1,652M | -46.9% |
| IPC | 3.65 | 4.20 | +15.1% |
| FPS | 4,666 | 9,799 | +110% |

Top hot functions (baseline): MasterClock::tick() 29.4%, GateArray::render_frame() 16.3%,
MemorySystem::read() 8.0%, run_frame() 5.8%, CPU6809::execute_page1() 5.5%.
Cross-TU call overhead totals ~42% of cycles. LTO eliminates it automatically.

See `doc/profiling-results.md` for full analysis and flame graphs.

## Task 1: Build Infrastructure

- [x] 1.1 Add `crayon_benchmark` target to `CMakeLists.txt` under `BUILD_TESTS`, linked against `crayon_core`
- [x] 1.2 Add `profile` preset to `CMakePresets.json` with `-O2 -g -fno-omit-frame-pointer` flags, Ninja generator, `build/profile` output
- [x] 1.3 Add `release-lto` preset to `CMakePresets.json` with `-O2 -flto` flags, Ninja generator, `build/release-lto` output
- [x] 1.4 Add `pgo-generate` preset to `CMakePresets.json` with `-O2 -fprofile-generate` flags
- [x] 1.5 Add `pgo-use` preset to `CMakePresets.json` with `-O2 -fprofile-use` flags
- [x] 1.6 Verify `cmake --preset profile` configures successfully and `cmake --build build/profile --target crayon_benchmark` produces a working binary

## Task 2: Baseline Profiling (GATE — completed)

- [x] 2.1 Run `perf stat` baseline: 851.8M cycles, 3.11B instructions, 3.65 IPC, 4666 FPS
- [x] 2.2 Generate flame graphs (baseline + LTO) with `perf record` + `inferno-flamegraph`
- [x] 2.3 Identify hot functions: MasterClock::tick() 29.4%, GateArray::render_frame() 16.3%, MemorySystem::read() 8.0%
- [x] 2.4 Create `perf-baseline.md` and `doc/profiling-results.md` with full analysis
- [x] 2.5 Run LTO comparison: 393.4M cycles (-53.8%), confirming cross-TU call overhead as primary bottleneck

## Task 3: Enable LTO for Release Builds (primary optimization — zero code changes)

- [x] 3.1 Add `ci-linux` preset to `CMakePresets.json` inheriting from `release-lto`, targeting Linux GCC release builds
- [x] 3.2 Verify `cmake --preset ci-linux` + build + benchmark produces ~393M cycles / ~9800 FPS
- [x] 3.3 Run `perf stat` and record: 392.8M cycles, 4.21 IPC, -53.9% vs baseline
    - _Requirements: 1.1, 1.2, 9.1, 9.2, 9.3_

## Task 4: MasterClock Accessor Inlining (for non-LTO builds)

- [ ] 4.1 Move `tick()`, `frame_complete()`, `clear_frame_complete()`, `get_cycle_count()`, `get_current_scanline()`, `get_scanline_cycle()` bodies from `src/master_clock.cpp` to `include/master_clock.h` as inline definitions. Keep `reset()` and constructor in `.cpp`.
- [ ] 4.2 Write RapidCheck property test for Property 1 (MasterClock tick state determinism) in `tests/test_perf_properties.cpp`
- [ ] 4.3 Verify build succeeds and all existing tests pass
- [x] 4.4 Run `perf stat` on profile build (non-LTO) and record cycle delta vs baseline. Result: 710.1M cycles, -16.6% vs baseline (851.8M). IPC: 4.01.
    - _Requirements: 3.1, 3.2, 3.3_

## Task 5: PIA + AudioSystem Inlining (for non-LTO builds)

- [x] 5.1 Move `PIA::irq_active()` and `PIA::firq_active()` bodies from `src/pia.cpp` to `include/pia.h` as inline definitions
- [x] 5.2 Move `AudioSystem::tick()` body from `src/audio_system.cpp` to `include/audio_system.h` as inline definition
- [x] 5.3 Write RapidCheck property test for Property 2 (PIA interrupt flag correctness) in `tests/test_perf_properties.cpp`
- [x] 5.4 Write RapidCheck property test for Property 7 (AudioSystem sample count preservation) in `tests/test_perf_properties.cpp`
- [x] 5.5 Verify build succeeds and all existing tests pass
- [x] 5.6 Run `perf stat` on profile build: 660.8M cycles, -22.4% cumulative vs baseline (851.8M). IPC: 4.11.
    - _Requirements: 4.1, 4.2, 4.3, 8.1, 8.2, 8.3_

## Task 6: run_frame() Fast Path + Cassette Skip (algorithmic — benefits both LTO and non-LTO)

- [ ] 6.1 Hoist `cassette_.get_load_mode()` check before the while loop in `EmulatorCore::run_frame()`. Guard the cassette intercept block with the hoisted boolean so `cpu_.get_pc()` is not called when cassette is not in fast mode.
- [ ] 6.2 Hoist `cassette_.is_playing() || cassette_.is_recording()` check before the while loop. Guard `cassette_.update_cycle()` with the hoisted `cassette_active` boolean.
- [ ] 6.3 Write RapidCheck property test for Property 3 (run_frame fast path equivalence) in `tests/test_perf_properties.cpp`
- [ ] 6.4 Verify build succeeds and all existing tests pass
- [x] 6.5 Run `perf stat` on both profile and LTO builds: profile 608.8M cycles (-28.5% cumulative), IPC 4.31.
    - _Requirements: 5.1, 5.2, 5.3, 5.4, 6.1, 6.2, 6.3_

## Task 7: Palette LUT for Libretro Framebuffer (algorithmic — benefits both builds)

- [ ] 7.1 Add `constexpr uint32 MO5_PALETTE_XRGB8888[16]` to `include/types.h` with precomputed XRGB8888 values
- [ ] 7.2 Add `bool xrgb_mode_` field to `GateArrayState` (default `false`) and `set_palette_mode(bool)` method to `GateArray`
- [ ] 7.3 Modify `GateArray::render_frame()` to select `MO5_PALETTE_XRGB8888` or `MO5_PALETTE_RGBA` based on `xrgb_mode_`
- [ ] 7.4 In `src/libretro.cpp` `retro_load_game()`, call `g_emulator->get_gate_array().set_palette_mode(true)`
- [ ] 7.5 In `src/libretro.cpp` `retro_run()`, replace `rgba_to_xrgb()` loop with direct `memcpy` (non-VKB) or direct copy (VKB visible)
- [ ] 7.6 Write RapidCheck property tests for Property 5 (palette round-trip) and Property 6 (GateArray palette mode output)
- [x] 7.7 Verify build succeeds and all existing tests pass
    - _Requirements: 7.1, 7.2, 7.3, 7.4, 7.5_

## Task 8: Final Measurement + Regression Gate

- [x] 8.1 Run final `perf stat` on both profile and LTO builds:
    - Profile (-O2, manual inlining + algorithmic): 601.1M cycles, -29.4% vs baseline
    - LTO (-O2 -flto + algorithmic): 399.0M cycles, -53.1% vs baseline, 10289 FPS
- [ ] 8.2 Run `crayon_benchmark --check --baseline-fps <optimized_fps>` to verify regression gate
- [ ] 8.3 Update `doc/profiling-results.md` with final optimized numbers and cumulative deltas
    - _Requirements: 10.1, 10.2, 10.3, 11.1, 11.2, 11.3, 11.4_
