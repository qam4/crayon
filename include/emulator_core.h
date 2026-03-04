#ifndef CRAYON_EMULATOR_CORE_H
#define CRAYON_EMULATOR_CORE_H

#include "types.h"
#include "cpu6809.h"
#include "gate_array.h"
#include "memory_system.h"
#include "pia.h"
#include "audio_system.h"
#include "input_handler.h"
#include "light_pen.h"
#include "cassette_interface.h"
#include "master_clock.h"
#include <string>

namespace crayon {

class Debugger;

struct Configuration {
    std::string basic_rom_path;
    std::string monitor_rom_path;
    bool enable_profile = false;

    Configuration() = default;
};

class EmulatorCore {
public:
    explicit EmulatorCore(const Configuration& config);
    ~EmulatorCore() = default;

    // ROM loading
    Result<void> load_roms(const std::string& basic_path,
                           const std::string& monitor_path);
    Result<void> load_basic_rom(const std::string& path);
    Result<void> load_basic_rom(const uint8* data, size_t size);
    Result<void> load_monitor_rom(const std::string& path);
    Result<void> load_monitor_rom(const uint8* data, size_t size);
    Result<void> load_cartridge(const std::string& path);
    Result<void> load_cartridge(const uint8* data, size_t size);

    // Emulation control
    void reset();
    void run_frame();
    void step();

    // Output
    const uint32* get_framebuffer() const;
    void get_audio_buffer(int16* buffer, size_t samples);

    // Input
    InputHandler& get_input_handler() { return input_; }

    // State
    Result<void> save_state(const std::string& path);
    Result<void> load_state(const std::string& path);

    bool is_running() const { return running_; }
    bool is_paused() const { return paused_; }
    void set_paused(bool paused) { paused_ = paused; }
    uint64 get_frame_count() const { return frame_count_; }

    // Component access
    CPU6809& get_cpu() { return cpu_; }
    GateArray& get_gate_array() { return gate_array_; }
    MemorySystem& get_memory() { return memory_; }
    PIA& get_pia() { return pia_; }
    AudioSystem& get_audio() { return audio_; }
    MasterClock& get_master_clock() { return master_clock_; }
    CassetteInterface& get_cassette() { return cassette_; }
    void play_cassette() { cassette_.play(master_clock_.get_cycle_count()); }
    void rewind_cassette() { cassette_.rewind(master_clock_.get_cycle_count()); }
    LightPen& get_light_pen() { return light_pen_; }

    // State access
    CPU6809State get_cpu_state() const { return cpu_.get_state(); }
    MO5MemoryState get_memory_state() const { return memory_.get_state(); }
    GateArrayState get_gate_array_state() const { return gate_array_.get_state(); }
    PIAState get_pia_state() const { return pia_.get_state(); }

    // Debugger
    void set_debugger(Debugger* debugger) { debugger_ = debugger; }
    Debugger* get_debugger() { return debugger_; }

private:
    Configuration config_;
    CPU6809 cpu_;
    GateArray gate_array_;
    MemorySystem memory_;
    PIA pia_;
    AudioSystem audio_;
    InputHandler input_;
    LightPen light_pen_;
    CassetteInterface cassette_;
    MasterClock master_clock_;
    Debugger* debugger_ = nullptr;
    bool running_ = false;
    bool paused_ = false;
    uint64 frame_count_ = 0;

    void handle_interrupts();
};

} // namespace crayon

#endif // CRAYON_EMULATOR_CORE_H
