#include "emulator_core.h"
#include "savestate.h"
#include "debugger.h"

namespace crayon {

EmulatorCore::EmulatorCore(const Configuration& config) : config_(config) {
    // Wire up components
    cpu_.set_memory_system(&memory_);
    memory_.set_pia(&pia_);
    memory_.set_gate_array(&gate_array_);
    memory_.set_cassette(&cassette_);
    memory_.set_audio(&audio_);
    pia_.set_input_handler(&input_);
    pia_.set_cassette(&cassette_);
    pia_.set_light_pen(&light_pen_);
    pia_.set_audio(&audio_);
    pia_.set_memory(&memory_);
}

Result<void> EmulatorCore::load_roms(const std::string& basic_path,
                                     const std::string& monitor_path) {
    auto result = memory_.load_basic_rom(basic_path);
    if (result.is_err()) return result;
    return memory_.load_monitor_rom(monitor_path);
}

Result<void> EmulatorCore::load_basic_rom(const std::string& path) { return memory_.load_basic_rom(path); }
Result<void> EmulatorCore::load_basic_rom(const uint8* data, size_t size) { return memory_.load_basic_rom(data, size); }
Result<void> EmulatorCore::load_monitor_rom(const std::string& path) { return memory_.load_monitor_rom(path); }
Result<void> EmulatorCore::load_monitor_rom(const uint8* data, size_t size) { return memory_.load_monitor_rom(data, size); }
Result<void> EmulatorCore::load_cartridge(const std::string& path) { return memory_.load_cartridge(path); }
Result<void> EmulatorCore::load_cartridge(const uint8* data, size_t size) { return memory_.load_cartridge(data, size); }

void EmulatorCore::reset() {
    cpu_.reset();
    gate_array_.reset();
    pia_.reset();
    audio_.reset();
    input_.reset();
    light_pen_.reset();
    cassette_.reset();
    master_clock_.reset();
    running_ = true;
    paused_ = false;
    frame_count_ = 0;
}

void EmulatorCore::run_frame() {
    if (!running_ || paused_) return;

    master_clock_.clear_frame_complete();

    // Hoist cassette state checks before the hot loop — these don't change mid-frame.
    const bool fast_cassette = !cassette_.get_state().k7_data.empty() &&
                               (cassette_.get_load_mode() == CassetteLoadMode::Fast);
    const bool cassette_active = cassette_.is_playing() || cassette_.is_recording();

    while (!master_clock_.frame_complete()) {
        // Fast cassette loading: intercept at TWO levels:
        //
        // 1. $F10B (leader search): The block-read routine at $F0FF starts
        //    by calling $F168 bit-by-bit to find leader bytes. We skip this
        //    by consuming raw K7 bytes until we find 0x01, then jump to
        //    $F118 (byte-level leader skip loop that calls $F181).
        //
        // 2. $F181 (read-one-byte): Called for leader skip, sync detection,
        //    and data reads. We inject the next byte from raw K7 data.
        //
        // Together these let the ROM's block framing logic run naturally
        // while bypassing all bit-level polling.

        if (fast_cassette) {
            uint16_t pc = cpu_.get_pc();

            if (pc == 0xF10B) {
                // Skip leader: find 0x3C sync in raw K7 data.
                // For standard K7 (0x3C followed by 0x5A): rewind before 0x3C
                // and let the ROM handle sync naturally via $F118/$F181.
                // For non-standard K7 (0x3C NOT followed by 0x5A): consume
                // the sync and jump directly to $F128 (block type read).
                uint8_t byte;
                bool found = false;
                bool has_5a = false;
                while (cassette_.try_fast_read_byte(byte)) {
                    if (byte == 0x3C) {
                        // Check if 0x5A follows
                        uint8_t peek;
                        if (cassette_.try_fast_read_byte(peek)) {
                            if (peek == 0x5A) {
                                has_5a = true;
                                // Rewind past both 0x3C and 0x5A
                                cassette_.rewind_fast_read(2);
                            } else {
                                // No 0x5A — rewind just the peek
                                cassette_.rewind_fast_read(1);
                            }
                        }
                        found = true;
                        break;
                    }
                }
                if (found) {
                    auto cpu_state = cpu_.get_state();
                    if (has_5a) {
                        // Standard format: let ROM handle sync via $F118
                        memory_.write(0x2045, 0x01);
                        cpu_state.a = 0x01;
                        cpu_state.pc = 0xF118;
                    } else {
                        // Non-standard: skip sync, jump to block type read
                        cpu_state.pc = 0xF128;
                        cpu_state.cc |= 0xD0;
                    }
                    cpu_state.clock_cycles += 4;
                    cpu_.set_state(cpu_state);
                    for (uint8_t i = 0; i < 4; ++i)
                        master_clock_.tick();
                }
                continue;
            }

            if (pc == 0xF181) {
                uint8_t byte;
                if (cassette_.try_fast_read_byte(byte)) {
                    auto cpu_state = cpu_.get_state();
                    cpu_state.a = byte;
                    cpu_state.b = 0;
                    cpu_state.pc = 0xF18A;
                    cpu_state.clock_cycles += 10;
                    cpu_.set_state(cpu_state);
                    memory_.write(0x2045, byte);
                    for (uint8_t i = 0; i < 10; ++i)
                        master_clock_.tick();
                    continue;
                }
            }
        }

        uint8_t cycles = cpu_.execute_instruction();
        for (uint8_t i = 0; i < cycles; ++i)
            master_clock_.tick();

        audio_.tick(cycles);
        if (cassette_active)
            cassette_.update_cycle(master_clock_.get_cycle_count());
        handle_interrupts();
    }

    // Signal VSYNC — sets irqb1_flag in PIA. The IRQ handler will run
    // naturally during the next frame's instruction loop when the CPU
    // checks interrupts after each instruction. The handler at $F657
    // reads Port B ($A7C1) which clears irqb1_flag, then does cursor work.
    pia_.signal_vsync();

    // Render the frame
    gate_array_.render_frame(memory_.get_pixel_ram(), memory_.get_color_ram());
    audio_.generate_samples(MasterClock::CYCLES_PER_FRAME);
    frame_count_++;
}

void EmulatorCore::step() {
    cpu_.execute_instruction();
}

const uint32* EmulatorCore::get_framebuffer() const {
    return gate_array_.get_framebuffer();
}

void EmulatorCore::get_audio_buffer(int16* buffer, size_t samples) {
    audio_.fill_audio_buffer(buffer, samples);
}

Result<void> EmulatorCore::save_state(const std::string& path) {
    SaveState state;
    state.cpu_state = cpu_.get_state();
    state.gate_array_state = gate_array_.get_state();
    state.memory_state = memory_.get_state();
    state.pia_state = pia_.get_state();
    state.audio_state = audio_.get_state();
    state.input_state = input_.get_state();
    state.light_pen_state = light_pen_.get_state();
    state.cassette_state = cassette_.get_state();
    state.frame_count = frame_count_;
    return SaveStateManager::save(path, state);
}

Result<void> EmulatorCore::load_state(const std::string& path) {
    auto result = SaveStateManager::load(path);
    if (result.is_err()) return Result<void>::err(result.error);
    auto state = *result.value;
    cpu_.set_state(state.cpu_state);
    gate_array_.set_state(state.gate_array_state);
    memory_.set_state(state.memory_state);
    pia_.set_state(state.pia_state);
    audio_.set_state(state.audio_state);
    input_.set_state(state.input_state);
    light_pen_.set_state(state.light_pen_state);
    cassette_.set_state(state.cassette_state);
    frame_count_ = state.frame_count;
    return Result<void>::ok();
}

void EmulatorCore::handle_interrupts() {
    // MO5 interrupt wiring (confirmed by ROM disassembly):
    //   PIA IRQA (CA1 = light pen) → active but unused by default handler
    //   PIA IRQB (CB1 = vsync)    → active, drives the cursor/frame counter
    // Both PIA IRQ outputs connect to the CPU IRQ line (active-OR).
    // The FIRQ vector ($FFF6) points to a stub that just does RTI.
    // The IRQ vector ($FFF8) points to $F657 — the real vsync handler
    // that increments the frame counter, checks cursor enable, and draws cursor.
    cpu_.assert_irq(pia_.irq_active() || pia_.firq_active());
    cpu_.assert_firq(false);
}

} // namespace crayon
