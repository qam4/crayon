#include "frontend.h"
#include "frontend_headless.h"
#include <fstream>
#include <iostream>
#include <string>
#include <memory>
#include <filesystem>

#ifdef ENABLE_SDL
#include "frontend_sdl.h"
#endif

void print_usage(const char* program) {
    std::cout << "Crayon - Thomson MO5 Emulator\n"
              << "Usage: " << program << " [options]\n"
              << "Options:\n"
              << "  --basic <path>     Path to BASIC ROM (12KB)\n"
              << "  --monitor <path>   Path to Monitor ROM (4KB)\n"
              << "  --cartridge <path> Path to cartridge ROM\n"
              << "  --cassette <path>  Path to K7 cassette file\n"
              << "  --headless         Run without display\n"
              << "  --frames <n>       Run for N frames then exit\n"
              << "  --screenshot <path> Save PNG screenshot after run (headless)\n"
              << "  --scale <n>        Display scale (1-4, default 2)\n"
              << "  --debugger         Enable debugger (F5 to toggle)\n"
              << "  --trace <path>     Dump per-frame trace to file (headless)\n"
              << "  --type <text>      Inject keystrokes in headless mode (use \\n for ENTER)\n"
              << "  --type-file <path> Read keystrokes from file (avoids shell quoting issues)\n"
              << "  --type-delay <n>   Frames to wait before typing (default 60)\n"
              << "  --help             Show this help\n";
}

int main(int argc, char* argv[]) {
    // Ensure output directories exist
    std::filesystem::create_directories("userdata");
    std::filesystem::create_directories("screenshots");

    crayon::FrontendConfig config;
    bool headless = false;
    int frame_limit = 0;
    std::string screenshot_path;
    std::string trace_path;
    std::string type_text;
    int type_delay = 60;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--help" || arg == "-h") { print_usage(argv[0]); return 0; }
        else if (arg == "--basic" && i + 1 < argc) config.basic_rom_path = argv[++i];
        else if (arg == "--monitor" && i + 1 < argc) config.monitor_rom_path = argv[++i];
        else if (arg == "--cartridge" && i + 1 < argc) config.cartridge_path = argv[++i];
        else if (arg == "--cassette" && i + 1 < argc) config.cassette_path = argv[++i];
        else if (arg == "--headless") headless = true;
        else if (arg == "--frames" && i + 1 < argc) frame_limit = std::stoi(argv[++i]);
        else if (arg == "--scale" && i + 1 < argc) config.display_scale = std::stoi(argv[++i]);
        else if (arg == "--screenshot" && i + 1 < argc) screenshot_path = argv[++i];
        else if (arg == "--debugger") config.enable_debugger = true;
        else if (arg == "--trace" && i + 1 < argc) trace_path = argv[++i];
        else if (arg == "--type" && i + 1 < argc) {
            type_text = argv[++i];
            // Replace literal \n with newline
            std::string processed;
            for (size_t j = 0; j < type_text.size(); ++j) {
                if (j + 1 < type_text.size() && type_text[j] == '\\' && type_text[j+1] == 'n') {
                    processed += '\n';
                    ++j;
                } else {
                    processed += type_text[j];
                }
            }
            type_text = processed;
        }
        else if (arg == "--type-file" && i + 1 < argc) {
            std::ifstream tf(argv[++i]);
            if (tf) { type_text.assign((std::istreambuf_iterator<char>(tf)), {}); }
            else { std::cerr << "Cannot open type-file: " << argv[i] << "\n"; return 1; }
        }
        else if (arg == "--type-delay" && i + 1 < argc) type_delay = std::stoi(argv[++i]);
        else { std::cerr << "Unknown option: " << arg << "\n"; print_usage(argv[0]); return 1; }
    }

    std::unique_ptr<crayon::Frontend> frontend;

    if (headless) {
        auto hf = std::make_unique<crayon::HeadlessFrontend>();
        if (frame_limit > 0) hf->set_frame_limit(frame_limit);
        if (!trace_path.empty()) hf->set_trace_path(trace_path);
        if (!type_text.empty()) hf->set_type_string(type_text, type_delay);
        frontend = std::move(hf);
    } else {
#ifdef ENABLE_SDL
        auto sf = std::make_unique<crayon::SDLFrontend>();
        if (frame_limit > 0) sf->set_frame_limit(frame_limit);
        frontend = std::move(sf);
#else
        std::cerr << "SDL not available. Use --headless or rebuild with SDL2.\n";
        return 1;
#endif
    }

    if (!frontend->initialize(config)) {
        std::cerr << "Failed to initialize frontend.\n";
        return 1;
    }

    frontend->run();
    if (!screenshot_path.empty()) {
        frontend->save_screenshot(screenshot_path);
        std::cout << "Screenshot saved to " << screenshot_path << "\n";
    }
    frontend->shutdown();
    return 0;
}
