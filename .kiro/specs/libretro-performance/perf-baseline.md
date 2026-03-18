# Crayon MO5 — Baseline Performance Profile

## Test Configuration

- Build: `profile` preset (`-O2 -g -fno-omit-frame-pointer`, GCC 7, Linux x86_64)
- Workload: 1000 frames, 100 warmup, BASIC prompt (no K7/cartridge loaded)
- ROMs: `roms/basic5.rom`, `roms/mo5.rom`
- Command: `perf stat -e cycles,instructions,cache-references,cache-misses,branches,branch-misses -- ./build/profile/crayon_benchmark --basic roms/basic5.rom --monitor roms/mo5.rom --frames 1000 --warmup 100`

## Baseline perf stat

| Counter | Value | Notes |
|---------|-------|-------|
| cycles | 851,790,800 | **Primary metric** |
| instructions | 3,109,951,039 | 3.65 IPC |
| cache-references | 6,128,527 | |
| cache-misses | 17,167 | 0.28% of cache refs |
| branches | 606,399,134 | |
| branch-misses | 0 | 0.00% — branch predictor is perfect |
| Wall clock | 0.214s | 4666 FPS, 93.3x real-time |

## Top Functions by Self Time (perf report --no-children)

| Rank | Function | Self % | Category |
|------|----------|--------|----------|
| 1 | MasterClock::tick() | 29.43% | Clock — **#1 target** |
| 2 | GateArray::render_frame() | 16.34% | Video |
| 3 | MemorySystem::read() | 7.99% | Memory |
| 4 | EmulatorCore::run_frame() | 5.75% | Frame loop overhead |
| 5 | CPU6809::execute_page1() | 5.51% | CPU |
| 6 | CPU6809::execute_instruction() | 5.47% | CPU |
| 7 | CPU6809::check_interrupts() | 3.39% | CPU |
| 8 | CPU6809::set_flag() | 3.12% | CPU |
| 9 | CPU6809::fetch() | 2.86% | CPU |
| 10 | AudioSystem::tick() | 2.86% | Audio |
| 11 | handle_interrupts() | 1.95% | Interrupts |
| 12 | CassetteInterface::update_cycle() | 1.56% | Cassette |
| 13 | CPU6809::assert_firq() | 1.35% | CPU |
| 14 | CPU6809::assert_irq() | 1.33% | CPU |
| 15 | CassetteInterface::get_load_mode() | 1.31% | Cassette |
| 16 | CPU6809::read() | 1.22% | CPU |
| 17 | PIA::firq_active() | 1.04% | PIA |
| 18 | PIA::irq_active() | 1.03% | PIA |
| 19 | MasterClock::frame_complete() | 1.02% | Clock |
| 20 | CPU6809::index_register() | 1.02% | CPU |

## Inclusive Cost (Children) for run_frame()

run_frame() total: 92.70% (self: 5.75%)
- MasterClock::tick(): 29.43%
- CPU6809::execute_instruction(): 28.56% (includes execute_page1, read, check_interrupts)
- GateArray::render_frame(): 16.34%
- handle_interrupts(): 3.60%
- AudioSystem::tick(): 2.86%
- CassetteInterface::update_cycle(): 1.56%
- CassetteInterface::get_load_mode(): 1.31%
- MasterClock::frame_complete(): 1.02%
- MasterClock::get_cycle_count(): 0.61%

## Analysis & Optimization Priority

### Key Observations

1. **MasterClock::tick() is 29.43%** — the single biggest target. This is pure function call
   overhead since tick() is just 3 increments + 2 comparisons. Moving to header for
   cross-TU inlining should eliminate most of this. Combined with frame_complete() (1.02%)
   and get_cycle_count() (0.61%), MasterClock accessors total ~31%.

2. **GateArray::render_frame() is 16.34%** — called once per frame. This is actual work
   (bitmap rendering), not call overhead. The palette LUT optimization won't reduce this
   much since it's already using array lookups. The RGBA→XRGB conversion happens in
   libretro.cpp which isn't in this benchmark path.

3. **Function call overhead is massive** — many small functions (AudioSystem::tick 2.86%,
   handle_interrupts 1.95%, CassetteInterface::update_cycle 1.56%, get_load_mode 1.31%,
   PIA::irq_active 1.03%, PIA::firq_active 1.04%, assert_irq 1.33%, assert_firq 1.35%)
   are trivial bodies dominated by call/return overhead. Total: ~12.4%.

4. **Branch prediction is perfect** (0.00% misses) — the CPU is not struggling with
   branches, it's struggling with function call overhead across translation units.

5. **IPC is 3.65** — very good, meaning the CPU pipeline is well-utilized. The bottleneck
   is instruction count, not pipeline stalls.

### Revised Optimization Priority (by expected impact)

| Priority | Optimization | Target % | Expected Reduction |
|----------|-------------|----------|-------------------|
| 1 | MasterClock inlining (tick + accessors) | ~31% | 15-25% cycles |
| 2 | CPU helper inlining (set_flag, fetch, assert_irq/firq) | ~10% | 5-8% cycles |
| 3 | AudioSystem::tick() inlining | 2.86% | 2-3% cycles |
| 4 | handle_interrupts + PIA inlining | ~4% | 2-3% cycles |
| 5 | Cassette skip (update_cycle + get_load_mode) | ~2.9% | 2-3% cycles |
| 6 | run_frame() fast path | ~5.75% self | 1-2% cycles |
| 7 | Palette LUT (libretro only) | N/A in benchmark | libretro-only gain |

## LTO Comparison (compiler cross-TU inlining)

| Metric | Profile (-O2) | LTO (-O2 -flto) | Delta |
|--------|--------------|-----------------|-------|
| cycles | 851,790,800 | 393,391,607 | **-53.8%** |
| instructions | 3,109,951,039 | 1,651,695,393 | **-46.9%** |
| IPC | 3.65 | 4.20 | +15.1% |
| FPS | 4,666 | 9,799 | **+110%** |

LTO eliminates cross-TU function call overhead automatically. The -53.8% cycle
reduction confirms that the majority of the profile build's overhead was indeed
function call overhead from trivial methods that the compiler couldn't inline
across translation units. This is NOT a build config deficiency — `-O2` is the
standard release optimization level. LTO is an additional optimization that
enables the linker to inline across TUs.

**Implication for manual inlining tasks:** Moving methods to headers achieves
the same effect as LTO but is more portable (works without `-flto`). The manual
inlining tasks (3, 4, 8) should target the same ~54% reduction. LTO gives us
the theoretical ceiling for cross-TU inlining gains.

### Surprise Finding: CPU Helper Functions

The profiling reveals that CPU6809 helper functions are significant:
- set_flag(): 3.12%
- fetch(): 2.86%
- assert_irq(): 1.33%
- assert_firq(): 1.35%
- read(): 1.22%
- index_register(): 1.02%

These weren't in the original optimization plan but total ~10.9%. Moving these
to the header could yield significant gains. Consider adding a task for CPU
helper inlining.
