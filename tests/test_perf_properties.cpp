// Feature: libretro-performance, Property 1: MasterClock tick state determinism

#include <gtest/gtest.h>
#include <rapidcheck.h>
#include "master_clock.h"

using namespace crayon;

// **Validates: Requirements 3.1**
// Property 1: For any N ticks from a reset state, the MasterClock state
// is deterministic and matches the expected formulas.
//
// Note: scanline_cycle_ counts modulo CYCLES_PER_SCANLINE (64) continuously
// and is NOT reset at frame boundaries. scanline_ IS reset at frame boundaries
// but increments based on scanline_cycle_ wraps. We compute expected values
// by simulating the two independent counters.
TEST(MasterClockProperty, TickStateDeterminism) {
    rc::check("tick state matches expected formulas for any N in [1, 100000]",
        []() {
            const uint32_t N = *rc::gen::inRange(1u, 100001u);

            MasterClock clock;
            clock.reset();

            for (uint32_t i = 0; i < N; ++i) {
                clock.tick();
            }

            // total_cycles_ == N
            RC_ASSERT(clock.get_cycle_count() == static_cast<uint64_t>(N));

            // frame_complete() is true iff N >= CYCLES_PER_FRAME (20000)
            const bool expected_frame_complete = (N >= MasterClock::CYCLES_PER_FRAME);
            RC_ASSERT(clock.frame_complete() == expected_frame_complete);

            // scanline_cycle_ counts modulo 64 continuously (independent of frame)
            const uint32_t expected_scanline_cycle = N % MasterClock::CYCLES_PER_SCANLINE;
            RC_ASSERT(clock.get_scanline_cycle() == expected_scanline_cycle);

            // For scanline_: compute by replaying the frame/scanline reset logic.
            // scanline_ resets to 0 at each frame boundary, then increments on
            // each scanline_cycle_ wrap. Since scanline_cycle_ is continuous,
            // we need to count wraps since the last frame reset.
            //
            // Approach: find the tick number of the last frame reset, then count
            // how many scanline_cycle_ wraps occurred between that tick and N.
            // The scanline_ value depends on the interplay of frame and
            // scanline counters. Use a reference simulation for correctness.
            uint32_t ref_scanline = 0;
            uint32_t ref_scanline_cycle = 0;
            uint32_t ref_frame_cycle = 0;
            for (uint32_t i = 0; i < N; ++i) {
                ref_frame_cycle++;
                ref_scanline_cycle++;
                if (ref_scanline_cycle >= MasterClock::CYCLES_PER_SCANLINE) {
                    ref_scanline_cycle = 0;
                    ref_scanline++;
                }
                if (ref_frame_cycle >= MasterClock::CYCLES_PER_FRAME) {
                    ref_frame_cycle = 0;
                    ref_scanline = 0;
                }
            }
            RC_ASSERT(clock.get_current_scanline() == ref_scanline);
        });
}

// Feature: libretro-performance, Property 2: PIA interrupt flag correctness

#include "pia.h"

// **Validates: Requirements 4.1**
// Property 2: For any PIAState with arbitrary irq flags and control register
// values, irq_active() and firq_active() must match the expected boolean
// expressions.
TEST(PIAProperty, InterruptFlagCorrectness) {
    rc::check("irq_active and firq_active match expected boolean expressions",
        []() {
            const bool irqa1 = *rc::gen::arbitrary<bool>();
            const bool irqa2 = *rc::gen::arbitrary<bool>();
            const uint8_t cra = *rc::gen::arbitrary<uint8_t>();
            const bool irqb1 = *rc::gen::arbitrary<bool>();
            const bool irqb2 = *rc::gen::arbitrary<bool>();
            const uint8_t crb = *rc::gen::arbitrary<uint8_t>();

            PIAState state{};
            state.irqa1_flag = irqa1;
            state.irqa2_flag = irqa2;
            state.cra = cra;
            state.irqb1_flag = irqb1;
            state.irqb2_flag = irqb2;
            state.crb = crb;

            PIA pia;
            pia.set_state(state);

            const bool expected_irq = (irqa1 && (cra & 0x01)) ||
                                      (irqa2 && (cra & 0x08));
            const bool expected_firq = (irqb1 && (crb & 0x01)) ||
                                       (irqb2 && (crb & 0x08));

            RC_ASSERT(pia.irq_active() == expected_irq);
            RC_ASSERT(pia.firq_active() == expected_firq);
        });
}

// Feature: libretro-performance, Property 7: AudioSystem sample count preservation

#include "audio_system.h"

// **Validates: Requirements 8.2**
// Property 7: For any partition of 20000 cycles into instruction-sized chunks
// (2-12 cycles each), calling tick() with each chunk then generate_samples()
// must produce the same number of samples as calling tick(20000) then
// generate_samples().
TEST(AudioSystemProperty, SampleCountPreservation) {
    rc::check("sample count is identical regardless of tick() partitioning",
        []() {
            // Generate a random partition of 20000 cycles into chunks of 2-12
            std::vector<int> chunks;
            int remaining = 20000;
            while (remaining > 0) {
                int max_chunk = std::min(remaining, 12);
                int min_chunk = std::min(remaining, 2);
                int chunk = *rc::gen::inRange(min_chunk, max_chunk + 1);
                chunks.push_back(chunk);
                remaining -= chunk;
            }

            // Partitioned path: tick each chunk, then generate_samples
            AudioSystem partitioned;
            partitioned.reset();
            for (int c : chunks) {
                partitioned.tick(c);
            }
            partitioned.generate_samples(0);
            size_t partitioned_samples = partitioned.samples_available();

            // Reference path: tick all at once, then generate_samples
            AudioSystem reference;
            reference.reset();
            reference.tick(20000);
            reference.generate_samples(0);
            size_t reference_samples = reference.samples_available();

            RC_ASSERT(partitioned_samples == reference_samples);
        });
}

// Feature: libretro-performance, Property 3: run_frame fast path guard correctness

#include "cassette_interface.h"

// **Validates: Requirements 5.1, 5.2**
// Property 3: The fast path guard `cassette_.has_data() && cassette_.get_load_mode() == Fast`
// correctly evaluates to false when no cassette is loaded, ensuring cpu_.get_pc()
// is never called in the common benchmark case. Additionally, the cassette_active
// guard `is_playing() || is_recording()` correctly evaluates to false after reset,
// ensuring update_cycle() is skipped when no cassette is active.
TEST(CassetteProperty, FastPathGuardCorrectness) {
    rc::check("fast path guard is false when no K7 loaded, true only when data present and mode is Fast",
        []() {
            CassetteInterface cassette;

            // After construction (no K7 loaded):
            // - get_load_mode() defaults to Fast
            // - has_data() returns false (no parsed blocks)
            // So the combined guard must be false.
            RC_ASSERT(cassette.get_load_mode() == CassetteLoadMode::Fast);
            RC_ASSERT(!cassette.has_data());

            const bool fast_cassette = cassette.has_data() &&
                                       (cassette.get_load_mode() == CassetteLoadMode::Fast);
            RC_ASSERT(!fast_cassette);

            // cassette_active guard: after construction, neither playing nor recording
            const bool cassette_active = cassette.is_playing() || cassette.is_recording();
            RC_ASSERT(!cassette_active);

            // After reset, same invariants hold
            cassette.reset();
            RC_ASSERT(!cassette.has_data());
            RC_ASSERT(cassette.get_load_mode() == CassetteLoadMode::Fast);
            RC_ASSERT(!cassette.is_playing());
            RC_ASSERT(!cassette.is_recording());

            // Switching to Slow mode: guard must also be false (mode != Fast)
            cassette.set_load_mode(CassetteLoadMode::Slow);
            const bool slow_guard = cassette.has_data() &&
                                    (cassette.get_load_mode() == CassetteLoadMode::Fast);
            RC_ASSERT(!slow_guard);

            // Switching back to Fast: still false because no data
            cassette.set_load_mode(CassetteLoadMode::Fast);
            const bool fast_no_data = cassette.has_data() &&
                                      (cassette.get_load_mode() == CassetteLoadMode::Fast);
            RC_ASSERT(!fast_no_data);
        });
}

// Feature: libretro-performance, Property 5: Palette RGBA-to-XRGB round-trip equivalence

#include "gate_array.h"
#include "types.h"
#include <set>

// Helper: RGBA → XRGB8888 conversion (same logic as libretro.cpp's rgba_to_xrgb)
static inline uint32_t test_rgba_to_xrgb(crayon::uint32 rgba) {
    uint32_t r = (rgba >> 24) & 0xFF;
    uint32_t g = (rgba >> 16) & 0xFF;
    uint32_t b = (rgba >>  8) & 0xFF;
    return (r << 16) | (g << 8) | b;
}

// **Validates: Requirements 7.2**
// Property 5: For all 16 palette indices, MO5_PALETTE_XRGB8888[i] must equal
// the result of applying rgba_to_xrgb() to MO5_PALETTE_RGBA[i].
TEST(PaletteProperty, RGBAtoXRGBRoundTrip) {
    rc::check("XRGB8888 palette entries match RGBA >> 8 for all 16 indices",
        []() {
            const int idx = *rc::gen::inRange(0, 16);
            const uint32_t expected = test_rgba_to_xrgb(crayon::MO5_PALETTE_RGBA[idx]);
            RC_ASSERT(crayon::MO5_PALETTE_XRGB8888[idx] == expected);
        });
}

// Feature: libretro-performance, Property 6: GateArray palette mode output correctness

// **Validates: Requirements 7.3, 7.4**
// Property 6: For any valid pixel_ram and color_ram (8000 bytes each),
// when xrgb_mode is true, every pixel in the rendered framebuffer must be
// a value present in MO5_PALETTE_XRGB8888[0..15]. When xrgb_mode is false,
// every pixel must be a value present in MO5_PALETTE_RGBA[0..15].
TEST(GateArrayProperty, PaletteModeOutputCorrectness) {
    rc::check("all rendered pixels belong to the correct palette for the active mode",
        []() {
            // Generate random pixel_ram and color_ram (8000 bytes each)
            auto pixel_ram = *rc::gen::container<std::vector<uint8_t>>(8000, rc::gen::arbitrary<uint8_t>());
            auto color_ram = *rc::gen::container<std::vector<uint8_t>>(8000, rc::gen::arbitrary<uint8_t>());

            // Build sets of valid palette values for quick lookup
            std::set<uint32_t> xrgb_set(std::begin(crayon::MO5_PALETTE_XRGB8888),
                                         std::end(crayon::MO5_PALETTE_XRGB8888));
            std::set<uint32_t> rgba_set(std::begin(crayon::MO5_PALETTE_RGBA),
                                         std::end(crayon::MO5_PALETTE_RGBA));

            // Test XRGB mode
            {
                crayon::GateArray ga;
                ga.set_palette_mode(true);
                ga.render_frame(pixel_ram.data(), color_ram.data());
                const uint32_t* fb = ga.get_framebuffer();
                for (int i = 0; i < crayon::DISPLAY_WIDTH * crayon::DISPLAY_HEIGHT; ++i) {
                    RC_ASSERT(xrgb_set.count(fb[i]) > 0);
                }
            }

            // Test RGBA mode
            {
                crayon::GateArray ga;
                ga.set_palette_mode(false);
                ga.render_frame(pixel_ram.data(), color_ram.data());
                const uint32_t* fb = ga.get_framebuffer();
                for (int i = 0; i < crayon::DISPLAY_WIDTH * crayon::DISPLAY_HEIGHT; ++i) {
                    RC_ASSERT(rgba_set.count(fb[i]) > 0);
                }
            }
        });
}
