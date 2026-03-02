#include "frontend.h"
#include "frontend_headless.h"
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
        else { std::cerr << "Unknown option: " << arg << "\n"; print_usage(argv[0]); return 1; }
    }

    std::unique_ptr<crayon::Frontend> frontend;

    if (headless) {
        auto hf = std::make_unique<crayon::HeadlessFrontend>();
        if (frame_limit > 0) hf->set_frame_limit(frame_limit);
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
