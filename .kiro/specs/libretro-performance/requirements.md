# Requirements Document

## Introduction

Performance optimization of the Crayon MO5 libretro core, following a profiling-first methodology. No code changes are permitted before establishing a baseline profile with Linux `perf`. Each optimization is validated individually against the baseline using deterministic cycle counts from `perf stat`. The approach mirrors the Videopac emulator optimization campaign that achieved -45% cycles / +34% FPS.

The MO5 inner loop runs 20,000 CPU cycles per frame (1 MHz CPU, 50 Hz PAL). The hot path is `EmulatorCore::run_frame()` which calls `CPU6809::execute_instruction()`, `MasterClock::tick()` (20K calls/frame), `AudioSystem::tick()`, `CassetteInterface::update_cycle()`, and `PIA::irq_active()`/`firq_active()` per instruction (~4000 calls/frame). The `GateArray::render_frame()` is called once per frame (bitmap renderer, not per-pixel). The libretro frontend converts RGBA→XRGB8888 per-pixel (64,000 pixels/frame).

## Glossary

- **Benchmark_Harness**: The `crayon_benchmark` tool (`tools/crayon_benchmark.cpp`) that runs N frames headlessly and reports FPS/ms-per-frame. Used for wall-clock regression checks.
- **Perf_Stat**: Linux `perf stat` command that reports hardware performance counters (CPU cycles, instructions, cache misses). Provides deterministic, load-independent measurements.
- **Perf_Record**: Linux `perf record` command that samples the call stack to produce flame graphs for identifying hot functions.
- **Baseline_Profile**: The initial `perf stat` cycle count and `perf record` flame graph captured before any code changes. All optimizations are measured as percentage reduction from this baseline.
- **Hot_Path**: The `EmulatorCore::run_frame()` inner loop: the `while (!master_clock_.frame_complete())` loop that executes ~4000 instructions and ~20,000 clock ticks per frame.
- **Cross_TU_Inlining**: Moving trivial method bodies from `.cpp` files to `.h` headers so the compiler can inline them across translation units (different `.cpp` files).
- **Palette_LUT**: A precomputed 16-entry lookup table mapping MO5 palette indices directly to XRGB8888 values, eliminating per-pixel RGBA→XRGB bit-shifting.
- **Fast_Path**: An optimized code path in `run_frame()` that skips unnecessary checks (debugger, cassette intercept) when those features are not active.
- **Profile_Build**: A CMake build preset with `-O2 -g -fno-omit-frame-pointer` flags that enables `perf record` to capture accurate call stacks while maintaining representative optimization levels.
- **LTO_Build**: A CMake build preset with Link-Time Optimization (`-flto`) that enables cross-TU inlining even without moving methods to headers.
- **EmulatorCore**: The `EmulatorCore` class (`src/emulator_core.cpp`) that orchestrates the MO5 emulation frame loop.
- **MasterClock**: The `MasterClock` class (`src/master_clock.cpp`) that tracks cycle/scanline/frame timing, called via `tick()` 20,000 times per frame.
- **GateArray**: The `GateArray` class (`src/gate_array.cpp`) that renders the MO5 320×200 bitmap framebuffer once per frame.
- **Libretro_Core**: The libretro frontend adapter (`src/libretro.cpp`) that converts the emulator framebuffer from RGBA to XRGB8888 and submits it to RetroArch.

## Requirements

### Requirement 1: Build Infrastructure for Profiling

**User Story:** As a developer, I want CMake build presets for profiling and LTO builds, so that I can capture accurate `perf` data and test link-time optimizations.

#### Acceptance Criteria

1. WHEN the developer configures with the `profile` preset, THE Build_System SHALL produce binaries compiled with `-O2 -g -fno-omit-frame-pointer` flags suitable for `perf record` stack sampling.
2. WHEN the developer configures with the `release-lto` preset, THE Build_System SHALL produce binaries compiled with `-O2 -flto` flags enabling link-time optimization.
3. THE Build_System SHALL include `crayon_benchmark` as a build target in `CMakeLists.txt`, linked against `crayon_core`.
4. WHEN the developer runs `cmake --build` with the `profile` preset, THE Build_System SHALL produce a `crayon_benchmark` executable that can be invoked with `perf stat` and `perf record`.

### Requirement 2: Baseline Profiling (Gate Requirement)

**User Story:** As a developer, I want to capture baseline performance data before any optimization, so that every subsequent change is measured against a known reference point.

#### Acceptance Criteria

1. THE Baseline_Profile SHALL be captured by running `perf stat -e cycles,instructions,cache-misses -- ./crayon_benchmark --basic <rom> --monitor <rom> --frames 1000 --warmup 100` on the unmodified codebase.
2. THE Baseline_Profile SHALL include a `perf record -g` flame graph identifying the top-5 hottest functions in the Hot_Path.
3. IF any optimization task is attempted before the Baseline_Profile is captured, THEN THE developer SHALL abort the optimization and capture the Baseline_Profile first.
4. THE Baseline_Profile cycle count and flame graph SHALL be recorded in a `perf-baseline.md` document in the spec directory for reference by all subsequent optimization tasks.

### Requirement 3: MasterClock Accessor Inlining

**User Story:** As a developer, I want MasterClock's trivially small methods inlined across translation units, so that the 20,000 calls per frame to `tick()` and related accessors avoid function call overhead.

#### Acceptance Criteria

1. WHEN the Profile_Build flame graph shows `MasterClock::tick()` consuming measurable cycles in the Hot_Path, THE EmulatorCore SHALL call `MasterClock::tick()` without cross-TU function call overhead by moving the method bodies of `tick()`, `frame_complete()`, `clear_frame_complete()`, `get_cycle_count()`, `get_current_scanline()`, and `get_scanline_cycle()` to `master_clock.h` as inline definitions.
2. THE MasterClock inlining SHALL be validated by a `perf stat` measurement showing a cycle count reduction compared to the Baseline_Profile.
3. IF the `perf stat` measurement shows no measurable cycle reduction, THEN THE optimization SHALL be reverted.

### Requirement 4: PIA Interrupt Check Inlining

**User Story:** As a developer, I want `PIA::irq_active()` and `PIA::firq_active()` inlined, so that the ~4000 calls per frame from `handle_interrupts()` avoid function call overhead.

#### Acceptance Criteria

1. WHEN the Profile_Build flame graph shows `PIA::irq_active()` or `PIA::firq_active()` consuming measurable cycles, THE EmulatorCore SHALL call these methods without cross-TU function call overhead by moving their bodies to `pia.h` as inline definitions.
2. THE PIA inlining SHALL be validated by a `perf stat` measurement showing a cycle count reduction compared to the previous optimization step.
3. IF the `perf stat` measurement shows no measurable cycle reduction, THEN THE optimization SHALL be reverted.

### Requirement 5: run_frame() Fast Path

**User Story:** As a developer, I want the `run_frame()` inner loop to skip cassette intercept checks when no cassette is loaded in fast mode, so that the common case (no cassette or slow-mode cassette) avoids two branch checks per iteration.

#### Acceptance Criteria

1. WHEN no cassette data is loaded or the cassette load mode is not `CassetteLoadMode::Fast`, THE EmulatorCore SHALL execute a fast path in `run_frame()` that skips the `cassette_.get_load_mode()` and `cpu_.get_pc()` checks in the inner loop.
2. WHEN a cassette is loaded in `CassetteLoadMode::Fast`, THE EmulatorCore SHALL execute the existing cassette intercept logic with identical behavior to the current implementation.
3. THE fast path SHALL be validated by a `perf stat` measurement showing a cycle count reduction for the no-cassette workload compared to the previous optimization step.
4. IF the `perf stat` measurement shows no measurable cycle reduction, THEN THE optimization SHALL be reverted.

### Requirement 6: CassetteInterface::update_cycle() Skip

**User Story:** As a developer, I want `CassetteInterface::update_cycle()` skipped when no cassette is playing, so that the ~4000 calls per frame avoid an unnecessary function call and assignment.

#### Acceptance Criteria

1. WHEN no cassette is playing (`is_playing()` returns false) and no cassette is recording, THE EmulatorCore SHALL skip the call to `CassetteInterface::update_cycle()` in the Hot_Path.
2. WHEN a cassette is playing or recording, THE EmulatorCore SHALL call `CassetteInterface::update_cycle()` with identical behavior to the current implementation.
3. THE cassette skip SHALL be validated by a `perf stat` measurement showing a cycle count reduction compared to the previous optimization step.

### Requirement 7: Palette LUT for Libretro Framebuffer

**User Story:** As a developer, I want the libretro core to use a precomputed 16-entry XRGB8888 palette lookup table, so that the GateArray can output XRGB8888 directly and the per-pixel `rgba_to_xrgb()` conversion (64,000 calls per frame) is eliminated.

#### Acceptance Criteria

1. THE Libretro_Core SHALL define a 16-entry `constexpr` array `MO5_PALETTE_XRGB8888` containing the MO5 palette in XRGB8888 format (0x00RRGGBB).
2. FOR ALL 16 palette entries, THE `MO5_PALETTE_XRGB8888[i]` value SHALL equal the result of applying `rgba_to_xrgb()` to `MO5_PALETTE_RGBA[i]` (round-trip equivalence).
3. WHEN the GateArray renders a frame for the libretro core, THE GateArray SHALL write XRGB8888 values directly to the framebuffer using the Palette_LUT, eliminating the per-pixel `rgba_to_xrgb()` conversion in `retro_run()`.
4. WHEN the GateArray renders a frame for the standalone SDL frontend, THE GateArray SHALL continue to write RGBA values to the framebuffer (no regression to existing behavior).
5. THE Palette_LUT optimization SHALL be validated by a `perf stat` measurement showing a cycle count reduction compared to the previous optimization step.

### Requirement 8: AudioSystem::tick() Batching

**User Story:** As a developer, I want `AudioSystem::tick()` calls batched per instruction rather than called with individual cycle counts, so that the function call overhead is reduced when the tick body is trivial (just incrementing a counter).

#### Acceptance Criteria

1. WHEN profiling shows `AudioSystem::tick()` consuming measurable cycles in the Hot_Path, THE EmulatorCore SHALL evaluate whether inlining `tick()` to the header or batching calls reduces overhead.
2. THE AudioSystem tick optimization SHALL preserve audio output correctness: the same number of audio samples SHALL be generated per frame as before the optimization.
3. THE AudioSystem optimization SHALL be validated by a `perf stat` measurement showing a cycle count reduction compared to the previous optimization step.
4. IF the `perf stat` measurement shows no measurable cycle reduction, THEN THE optimization SHALL be reverted.

### Requirement 9: CMake LTO/PGO Build Presets

**User Story:** As a developer, I want CMake presets for LTO and PGO builds, so that I can measure the impact of compiler-level cross-TU optimization and profile-guided optimization on the emulator.

#### Acceptance Criteria

1. WHEN the developer configures with the `release-lto` preset, THE Build_System SHALL enable `-flto` for all targets, allowing the compiler to inline across translation units without manual header moves.
2. WHEN the developer configures with the `pgo-generate` preset, THE Build_System SHALL compile with `-fprofile-generate` flags to produce instrumented binaries.
3. WHEN the developer configures with the `pgo-use` preset and provides a profile data directory, THE Build_System SHALL compile with `-fprofile-use` flags to apply profile-guided optimizations.
4. THE LTO build SHALL be validated by a `perf stat` measurement comparing cycle counts against the baseline and against the manually-inlined build.

### Requirement 10: Regression Benchmark Gate

**User Story:** As a developer, I want an automated benchmark check that fails if performance regresses below the optimized baseline, so that future changes do not accidentally undo performance gains.

#### Acceptance Criteria

1. WHEN the developer runs `crayon_benchmark --check --baseline-fps <N>`, THE Benchmark_Harness SHALL exit with a non-zero return code if the measured FPS is below the specified baseline.
2. THE Benchmark_Harness SHALL support a `--perf-cycles <N>` flag that records the expected cycle count from `perf stat`, enabling cycle-based regression detection in addition to FPS-based detection.
3. THE Benchmark_Harness SHALL output both human-readable and CSV formats for integration with CI pipelines.

### Requirement 11: Optimization Measurement Protocol

**User Story:** As a developer, I want a documented measurement protocol that enforces profiling before and after each optimization, so that every change has quantified evidence of its impact.

#### Acceptance Criteria

1. THE Measurement_Protocol SHALL require a `perf stat` cycle count measurement before and after each optimization, using identical workload parameters (1000 frames, 100 warmup, same ROM).
2. THE Measurement_Protocol SHALL require that each optimization is applied as an isolated commit, measured independently, before combining with other optimizations.
3. IF an optimization shows less than 1% cycle reduction in `perf stat`, THEN THE Measurement_Protocol SHALL flag the optimization as "not validated" and recommend reverting.
4. THE Measurement_Protocol SHALL record cumulative cycle reduction as a running total across all applied optimizations.
