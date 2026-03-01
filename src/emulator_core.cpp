#include "emulator_core.h"
#include "savestate.h"
#include "debugger.h"

namespace crayon {

EmulatorCore::EmulatorCore(const Configuration& config) : config_(config) {
    // Wire up components
    cpu_.set_memory_system(&memory_);
    memory_.set_pia(&pia_);
    memory_.set_gate_array(&gate_array_);
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
    while (!master_clock_.frame_complete()) {
        uint8_t cycles = cpu_.execute_instruction();
        for (uint8_t i = 0; i < cycles; ++i)
            master_clock_.tick();

        handle_interrupts();
    }

    // Render frame
    gate_array_.render_frame(memory_.get_pixel_ram(), memory_.get_color_ram());
    pia_.signal_vsync();
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
    bool firq_was_active = pia_.firq_active();
    cpu_.assert_irq(pia_.irq_active());
    cpu_.assert_firq(firq_was_active);

    // If FIRQ line was just asserted and CPU will take it, acknowledge
    // the vsync interrupt so it doesn't re-fire every instruction.
    // The CPU's check_interrupts() (called at end of execute_instruction)
    // will service the FIRQ on the next cycle. We clear the PIA flag now
    // so firq_active() returns false on subsequent calls this frame.
    if (firq_was_active) {
        pia_.acknowledge_firq();
    }
}

} // namespace crayon
