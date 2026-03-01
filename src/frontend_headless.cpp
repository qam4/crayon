#include "frontend_headless.h"
#include <iostream>
#include <fstream>

namespace crayon {

HeadlessFrontend::HeadlessFrontend() = default;
HeadlessFrontend::~HeadlessFrontend() { shutdown(); }

bool HeadlessFrontend::initialize(const FrontendConfig& config) {
    config_ = config;
    Configuration emu_config;
    emu_config.basic_rom_path = config.basic_rom_path;
    emu_config.monitor_rom_path = config.monitor_rom_path;
    emulator_ = std::make_unique<EmulatorCore>(emu_config);

    if (!config.basic_rom_path.empty()) {
        auto result = emulator_->load_basic_rom(config.basic_rom_path);
        if (result.is_err()) { std::cerr << "Failed to load BASIC ROM: " << result.error << "\n"; return false; }
    }
    if (!config.monitor_rom_path.empty()) {
        auto result = emulator_->load_monitor_rom(config.monitor_rom_path);
        if (result.is_err()) { std::cerr << "Failed to load Monitor ROM: " << result.error << "\n"; return false; }
    }
    if (!config.cartridge_path.empty()) {
        auto result = emulator_->load_cartridge(config.cartridge_path);
        if (result.is_err()) { std::cerr << "Failed to load cartridge: " << result.error << "\n"; return false; }
    }

    if (config.enable_debugger) {
        debugger_ = std::make_unique<Debugger>(emulator_.get());
        debugger_ui_ = std::make_unique<DebuggerUI>(debugger_.get());
        emulator_->set_debugger(debugger_.get());
    }

    emulator_->reset();
    running_ = true;
    return true;
}

void HeadlessFrontend::shutdown() { running_ = false; }

void HeadlessFrontend::run() {
    while (running_) {
        if (frame_limit_ > 0 && frame_count_ >= frame_limit_) { running_ = false; break; }
        emulator_->run_frame();
        frame_count_++;
    }
}

void HeadlessFrontend::render_frame() {}
void HeadlessFrontend::process_audio() {}
void HeadlessFrontend::process_input() {}
MenuAction HeadlessFrontend::process_menu() { return MenuAction::None; }
void HeadlessFrontend::show_message(const std::string& message) { std::cout << message << "\n"; }

void HeadlessFrontend::save_screenshot(const std::string& /*filename*/) {
    // TODO: Write framebuffer to PNG via stb_image_write
}

void HeadlessFrontend::dump_framebuffer(const std::string& filename) {
    const uint32* fb = emulator_->get_framebuffer();
    if (!fb) return;
    std::ofstream file(filename, std::ios::binary);
    // PPM format
    file << "P6\n" << DISPLAY_WIDTH << " " << DISPLAY_HEIGHT << "\n255\n";
    for (int i = 0; i < DISPLAY_WIDTH * DISPLAY_HEIGHT; ++i) {
        uint32 pixel = fb[i];
        uint8_t r = (pixel >> 24) & 0xFF;
        uint8_t g = (pixel >> 16) & 0xFF;
        uint8_t b = (pixel >> 8) & 0xFF;
        file.put(r); file.put(g); file.put(b);
    }
}

} // namespace crayon
