#ifndef CRAYON_FRONTEND_H
#define CRAYON_FRONTEND_H

#include "types.h"
#include <string>
#include <map>

namespace crayon {

class EmulatorCore;

struct FrontendConfig {
    int display_scale = 2;
    bool fullscreen = false;
    bool headless = false;
    std::string aspect_ratio = "4:3";

    int sample_rate = 44100;
    int audio_buffer_size = 512;
    float master_volume = 0.7f;
    bool audio_enabled = true;

    bool show_fps = true;
    bool enable_debugger = false;
    std::string trace_level;

    std::string basic_rom_path;
    std::string monitor_rom_path;
    std::string cartridge_path;
    std::string cassette_path;
    std::string k7_mode = "fast";  // "fast" or "slow"
    std::string save_state_path;
    std::string screenshot_path = "screenshots";
};

enum class MenuAction {
    None,
    LoadBasicROM,
    LoadMonitorROM,
    LoadCartridge,
    LoadK7,
    Reset,
    Pause,
    Resume,
    SaveState,
    LoadState,
    Screenshot,
    ToggleFPS,
    ToggleDebugger,
    ToggleFullscreen,
    Quit
};

class Frontend {
public:
    virtual ~Frontend() = default;

    virtual bool initialize(const FrontendConfig& config) = 0;
    virtual void shutdown() = 0;
    virtual void run() = 0;
    virtual bool is_running() const = 0;

    virtual void render_frame() = 0;
    virtual void process_audio() = 0;
    virtual void process_input() = 0;

    virtual MenuAction process_menu() = 0;
    virtual void show_message(const std::string& message) = 0;

    virtual EmulatorCore* get_emulator() = 0;

    virtual void save_screenshot(const std::string& filename) = 0;
    virtual void dump_framebuffer(const std::string& filename) = 0;
};

} // namespace crayon

#endif // CRAYON_FRONTEND_H
