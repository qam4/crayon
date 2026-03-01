#include "frontend_sdl.h"
#include "ui/text_renderer.h"
#include "ui/menu_system.h"
#include "ui/config_manager.h"
#include "ui/file_browser.h"
#include "ui/zip_handler.h"
#include "ui/dialogs.h"
#include "ui/recent_files_list.h"
#include "ui/save_state_manager.h"
#include <iostream>

namespace crayon {

SDLFrontend::SDLFrontend() = default;
SDLFrontend::~SDLFrontend() { shutdown(); }

bool SDLFrontend::initialize(const FrontendConfig& config) {
    config_ = config;
    if (!init_video()) return false;
    if (!init_audio()) return false;

    Configuration emu_config;
    emu_config.basic_rom_path = config.basic_rom_path;
    emu_config.monitor_rom_path = config.monitor_rom_path;
    emulator_ = std::make_unique<EmulatorCore>(emu_config);

    if (!config.basic_rom_path.empty()) emulator_->load_basic_rom(config.basic_rom_path);
    if (!config.monitor_rom_path.empty()) emulator_->load_monitor_rom(config.monitor_rom_path);
    if (!config.cartridge_path.empty()) emulator_->load_cartridge(config.cartridge_path);

    if (config.enable_debugger) {
        debugger_ = std::make_unique<Debugger>(emulator_.get());
        debugger_ui_ = std::make_unique<DebuggerUI>(debugger_.get());
        emulator_->set_debugger(debugger_.get());
        imgui_debugger_ui_ = std::make_unique<ImGuiDebuggerUI>(
            debugger_.get(), emulator_.get(), window_, renderer_);
        imgui_debugger_ui_->initialize();
    }

    emulator_->reset();
    running_ = true;
    return true;
}

void SDLFrontend::shutdown() {
    if (imgui_debugger_ui_) imgui_debugger_ui_->shutdown();
    cleanup_audio();
    cleanup_video();
    running_ = false;
}

void SDLFrontend::run() {
    while (running_) {
        if (frame_limit_ > 0 && frame_count_ >= frame_limit_) { running_ = false; break; }
        process_input();
        if (!paused_) emulator_->run_frame();
        render_frame();
        process_audio();
        frame_count_++;
    }
}

bool SDLFrontend::is_running() const { return running_; }

void SDLFrontend::render_frame() {
    if (!renderer_ || !texture_) return;
    const uint32* fb = emulator_->get_framebuffer();
    if (fb) SDL_UpdateTexture(texture_, nullptr, fb, DISPLAY_WIDTH * sizeof(uint32_t));
    SDL_RenderClear(renderer_);
    SDL_RenderCopy(renderer_, texture_, nullptr, nullptr);
    if (imgui_debugger_ui_ && imgui_debugger_ui_->is_visible())
        imgui_debugger_ui_->render();
    SDL_RenderPresent(renderer_);
}

void SDLFrontend::process_audio() {
    // TODO: Fill SDL audio buffer from emulator
}

void SDLFrontend::process_input() {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        if (imgui_debugger_ui_) imgui_debugger_ui_->process_event(event);
        switch (event.type) {
            case SDL_QUIT: running_ = false; break;
            case SDL_KEYDOWN: handle_keyboard_event(event.key); break;
            case SDL_KEYUP: handle_keyboard_event(event.key); break;
        }
    }
}

MenuAction SDLFrontend::process_menu() { return MenuAction::None; }
void SDLFrontend::show_message(const std::string& msg) { std::cout << msg << "\n"; }
EmulatorCore* SDLFrontend::get_emulator() { return emulator_.get(); }

void SDLFrontend::save_screenshot(const std::string& /*filename*/) {
    // TODO: Implement via stb_image_write
}

void SDLFrontend::dump_framebuffer(const std::string& /*filename*/) {
    // TODO: Implement
}

bool SDLFrontend::init_video() {
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) < 0) {
        std::cerr << "SDL_Init failed: " << SDL_GetError() << "\n";
        return false;
    }
    int scale = config_.display_scale;
    window_ = SDL_CreateWindow("Crayon - Thomson MO5",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        DISPLAY_WIDTH * scale, DISPLAY_HEIGHT * scale,
        SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);
    if (!window_) return false;
    renderer_ = SDL_CreateRenderer(window_, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!renderer_) return false;
    texture_ = SDL_CreateTexture(renderer_, SDL_PIXELFORMAT_RGBA8888,
        SDL_TEXTUREACCESS_STREAMING, DISPLAY_WIDTH, DISPLAY_HEIGHT);
    return texture_ != nullptr;
}

bool SDLFrontend::init_audio() {
    SDL_AudioSpec desired{};
    desired.freq = config_.sample_rate;
    desired.format = AUDIO_S16SYS;
    desired.channels = 1;
    desired.samples = config_.audio_buffer_size;
    desired.callback = audio_callback;
    desired.userdata = this;
    audio_device_ = SDL_OpenAudioDevice(nullptr, 0, &desired, nullptr, 0);
    if (audio_device_ > 0) SDL_PauseAudioDevice(audio_device_, 0);
    return audio_device_ > 0;
}

void SDLFrontend::cleanup_video() {
    if (texture_) { SDL_DestroyTexture(texture_); texture_ = nullptr; }
    if (renderer_) { SDL_DestroyRenderer(renderer_); renderer_ = nullptr; }
    if (window_) { SDL_DestroyWindow(window_); window_ = nullptr; }
}

void SDLFrontend::cleanup_audio() {
    if (audio_device_ > 0) { SDL_CloseAudioDevice(audio_device_); audio_device_ = 0; }
}

void SDLFrontend::handle_keyboard_event(const SDL_KeyboardEvent& event) {
    bool pressed = (event.type == SDL_KEYDOWN);
    // F5 = debugger toggle
    if (pressed && event.keysym.sym == SDLK_F5 && imgui_debugger_ui_) {
        if (imgui_debugger_ui_->is_visible()) imgui_debugger_ui_->hide();
        else imgui_debugger_ui_->show();
        return;
    }
    // Forward to input handler
    emulator_->get_input_handler().process_host_key(event.keysym.sym, pressed);
}

void SDLFrontend::handle_menu_action(MenuAction action) {
    switch (action) {
        case MenuAction::Reset: emulator_->reset(); break;
        case MenuAction::Pause: paused_ = true; break;
        case MenuAction::Resume: paused_ = false; break;
        case MenuAction::Quit: running_ = false; break;
        default: break;
    }
}

void SDLFrontend::audio_callback(void* userdata, uint8_t* stream, int len) {
    auto* self = static_cast<SDLFrontend*>(userdata);
    int samples = len / sizeof(int16_t);
    self->emulator_->get_audio_buffer(reinterpret_cast<int16*>(stream), samples);
}

} // namespace crayon
