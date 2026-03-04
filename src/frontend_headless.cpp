#include "frontend_headless.h"
#include "input_handler.h"
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

namespace crayon {

// Map ASCII char to MO5Key + whether SHIFT is needed
struct CharMapping {
    MO5Key key;
    bool shift;
};

static bool char_to_mo5(char c, CharMapping& out) {
    switch (c) {
        // Letters (MO5 boots in caps mode, no shift needed)
        case 'A': case 'a': out = {MO5Key::Q, false}; return true;   // AZERTY: A is at Q position
        case 'B': case 'b': out = {MO5Key::B, false}; return true;
        case 'C': case 'c': out = {MO5Key::C, false}; return true;
        case 'D': case 'd': out = {MO5Key::D, false}; return true;
        case 'E': case 'e': out = {MO5Key::E, false}; return true;
        case 'F': case 'f': out = {MO5Key::F, false}; return true;
        case 'G': case 'g': out = {MO5Key::G, false}; return true;
        case 'H': case 'h': out = {MO5Key::H, false}; return true;
        case 'I': case 'i': out = {MO5Key::I, false}; return true;
        case 'J': case 'j': out = {MO5Key::J, false}; return true;
        case 'K': case 'k': out = {MO5Key::K, false}; return true;
        case 'L': case 'l': out = {MO5Key::L, false}; return true;
        case 'M': case 'm': out = {MO5Key::SLASH, false}; return true; // MO5: M is at scancode 0x1A
        case 'N': case 'n': out = {MO5Key::N, false}; return true;
        case 'O': case 'o': out = {MO5Key::O, false}; return true;
        case 'P': case 'p': out = {MO5Key::P, false}; return true;
        case 'Q': case 'q': out = {MO5Key::A, false}; return true;   // AZERTY: Q is at A position
        case 'R': case 'r': out = {MO5Key::R, false}; return true;
        case 'S': case 's': out = {MO5Key::S, false}; return true;
        case 'T': case 't': out = {MO5Key::T, false}; return true;
        case 'U': case 'u': out = {MO5Key::U, false}; return true;
        case 'V': case 'v': out = {MO5Key::V, false}; return true;
        case 'W': case 'w': out = {MO5Key::Z, false}; return true;   // AZERTY: W is at Z position
        case 'X': case 'x': out = {MO5Key::X, false}; return true;
        case 'Y': case 'y': out = {MO5Key::Y, false}; return true;
        case 'Z': case 'z': out = {MO5Key::W, false}; return true;   // AZERTY: Z is at W position
        // Digits (no shift in caps mode on MO5)
        case '0': out = {MO5Key::Key0, false}; return true;
        case '1': out = {MO5Key::Key1, false}; return true;
        case '2': out = {MO5Key::Key2, false}; return true;
        case '3': out = {MO5Key::Key3, false}; return true;
        case '4': out = {MO5Key::Key4, false}; return true;
        case '5': out = {MO5Key::Key5, false}; return true;
        case '6': out = {MO5Key::Key6, false}; return true;
        case '7': out = {MO5Key::Key7, false}; return true;
        case '8': out = {MO5Key::Key8, false}; return true;
        case '9': out = {MO5Key::Key9, false}; return true;
        // Punctuation
        case ' ':  out = {MO5Key::SPACE, false}; return true;
        case '\n': out = {MO5Key::ENTER, false}; return true;
        case ',':  out = {MO5Key::M, false}; return true;      // MO5 scancode 0x08 produces ','
        case '.':  out = {MO5Key::COMMA, false}; return true;   // MO5 scancode 0x10 produces '.'
        case '"':  out = {MO5Key::Key2, true}; return true;     // SHIFT+2 = " on AZERTY
        case '-':  out = {MO5Key::MINUS, false}; return true;
        case '+':  out = {MO5Key::PLUS, false}; return true;
        case '*':  out = {MO5Key::STAR, false}; return true;
        case '/':  out = {MO5Key::SLASH, true}; return true;    // SHIFT+M position = /
        case '@':  out = {MO5Key::AT, false}; return true;
        default: return false;
    }
}

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
        std::cout << "Loaded BASIC ROM: " << config.basic_rom_path << "\n";
    }
    if (!config.monitor_rom_path.empty()) {
        auto result = emulator_->load_monitor_rom(config.monitor_rom_path);
        if (result.is_err()) { std::cerr << "Failed to load Monitor ROM: " << result.error << "\n"; return false; }
        std::cout << "Loaded Monitor ROM: " << config.monitor_rom_path << "\n";
    }
    if (!config.cartridge_path.empty()) {
        auto result = emulator_->load_cartridge(config.cartridge_path);
        if (result.is_err()) { std::cerr << "Failed to load cartridge: " << result.error << "\n"; return false; }
        std::cout << "Loaded cartridge: " << config.cartridge_path << "\n";
    }
    if (!config.cassette_path.empty()) {
        auto result = emulator_->get_cassette().load_k7(config.cassette_path);
        if (result.is_err()) { std::cerr << "Failed to load K7: " << result.error << "\n"; return false; }
        std::cout << "Loaded cassette: " << config.cassette_path << "\n";
    }

    if (config.enable_debugger) {
        debugger_ = std::make_unique<Debugger>(emulator_.get());
        debugger_ui_ = std::make_unique<DebuggerUI>(debugger_.get());
        emulator_->set_debugger(debugger_.get());
        std::cout << "Debugger enabled\n";
    }

    emulator_->reset();
    running_ = true;
    std::cout << "Headless frontend initialized\n";
    return true;
}

void HeadlessFrontend::shutdown() { running_ = false; }

void HeadlessFrontend::run() {
    std::ofstream trace_out;
    if (!trace_path_.empty()) {
        std::filesystem::create_directories(
            std::filesystem::path(trace_path_).parent_path());
        trace_out.open(trace_path_);
        trace_out << "=== Crayon Boot Trace ===\n";
    }

    while (running_) {
        if (frame_limit_ > 0 && frame_count_ >= frame_limit_) { running_ = false; break; }

        // Keystroke injection from --type
        if (!type_text_.empty() && frame_count_ >= type_delay_frames_ && type_index_ <= type_text_.size()) {
            auto& input = emulator_->get_input_handler();
            if (type_key_hold_frames_ > 0) {
                // Still holding current key
                type_key_hold_frames_--;
                if (type_key_hold_frames_ == 0) {
                    // Release all keys
                    input.reset();
                    type_index_++;
                    type_key_hold_frames_ = -3; // gap frames (negative = gap countdown)
                }
            } else if (type_key_hold_frames_ < 0) {
                // In gap between keys
                type_key_hold_frames_++;
            } else if (type_index_ < type_text_.size()) {
                // Press next key
                CharMapping mapping;
                if (char_to_mo5(type_text_[type_index_], mapping)) {
                    if (mapping.shift) input.set_key_state(MO5Key::SHIFT, true);
                    input.set_key_state(mapping.key, true);
                    type_key_hold_frames_ = 3;
                    if (trace_out.is_open()) {
                        trace_out << "  [type] char='" << type_text_[type_index_]
                                  << "' key=0x" << std::hex << (int)mapping.key
                                  << (mapping.shift ? " +SHIFT" : "") << "\n";
                    }
                } else {
                    // Unknown char, skip
                    type_index_++;
                }
            }
        }

        emulator_->run_frame();

        if (trace_out.is_open() && (frame_count_ < 60 || frame_count_ % 50 == 0)) {
            auto cpu = emulator_->get_cpu().get_state();
            auto pia = emulator_->get_pia().get_state();
            auto& mem = emulator_->get_memory();
            auto cass = emulator_->get_cassette().get_state();
            uint16_t firq_vec = (mem.read(0x2067) << 8) | mem.read(0x2068);
            trace_out << "Frame " << std::dec << frame_count_
                      << ": PC=$" << std::hex << std::setfill('0') << std::setw(4) << cpu.pc
                      << " CC=$" << std::setw(2) << (int)cpu.cc
                      << " (F=" << ((cpu.cc & 0x40) ? "1" : "0")
                      << " I=" << ((cpu.cc & 0x10) ? "1" : "0") << ")"
                      << " FIRQ_VEC=$" << std::setw(4) << firq_vec
                      << " frame_ctr=$" << std::setw(2) << (int)mem.read(0x2031)
                      << " cursor_en=$" << std::setw(2) << (int)mem.read(0x2063)
                      << " irqb1=" << pia.irqb1_flag
                      << " crb=$" << std::setw(2) << (int)pia.crb;
            if (cass.playing) {
                trace_out << " K7:playing pos=" << std::dec << cass.read_position
                          << "/" << cass.k7_data.size()
                          << " start_cy=" << cass.play_start_cycle
                          << " cur_cy=" << cass.current_cycle;
            } else if (!cass.k7_data.empty()) {
                trace_out << " K7:stopped";
            }
            trace_out << "\n";
        }

        frame_count_++;
    }

    if (trace_out.is_open()) {
        trace_out << "\nTrace complete: " << std::dec << frame_count_ << " frames\n";
        std::cout << "Trace written: " << trace_path_ << "\n";
    }
}

void HeadlessFrontend::render_frame() {}
void HeadlessFrontend::process_audio() {}
void HeadlessFrontend::process_input() {}
MenuAction HeadlessFrontend::process_menu() { return MenuAction::None; }
void HeadlessFrontend::show_message(const std::string& message) { std::cout << message << "\n"; }

void HeadlessFrontend::save_screenshot(const std::string& filename) {
    const uint32* fb = emulator_->get_framebuffer();
    if (!fb) return;
    // Convert RGBA uint32 to 4-byte-per-pixel array for stb
    // Our format is 0xRRGGBBAA, stb expects R,G,B,A bytes
    std::vector<uint8_t> pixels(320 * 200 * 4);
    for (int i = 0; i < 320 * 200; ++i) {
        uint32 p = fb[i];
        pixels[i * 4 + 0] = (p >> 24) & 0xFF; // R
        pixels[i * 4 + 1] = (p >> 16) & 0xFF; // G
        pixels[i * 4 + 2] = (p >> 8) & 0xFF;  // B
        pixels[i * 4 + 3] = p & 0xFF;          // A
    }
    stbi_write_png(filename.c_str(), 320, 200, 4, pixels.data(), 320 * 4);
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
