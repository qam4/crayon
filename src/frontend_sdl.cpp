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
    if (!config.cassette_path.empty()) {
        auto result = emulator_->get_cassette().load_k7(config.cassette_path);
        if (result.is_err()) { std::cerr << "Failed to load K7: " << result.error << "\n"; return false; }
        emulator_->get_cassette().play();
        std::cout << "Cassette loaded: " << config.cassette_path << "\n";
    }

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
            case SDL_MOUSEMOTION: {
                int w, h;
                SDL_GetWindowSize(window_, &w, &h);
                emulator_->get_light_pen().set_mouse_position(
                    event.motion.x, event.motion.y, w, h);
                break;
            }
            case SDL_MOUSEBUTTONDOWN:
                emulator_->get_light_pen().set_button_pressed(true);
                break;
            case SDL_MOUSEBUTTONUP:
                emulator_->get_light_pen().set_button_pressed(false);
                break;
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
    if (!window_) { std::cerr << "SDL_CreateWindow failed: " << SDL_GetError() << "\n"; return false; }
    renderer_ = SDL_CreateRenderer(window_, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!renderer_) {
        std::cerr << "Accelerated renderer unavailable, trying software: " << SDL_GetError() << "\n";
        renderer_ = SDL_CreateRenderer(window_, -1, SDL_RENDERER_SOFTWARE);
    }
    if (!renderer_) { std::cerr << "SDL_CreateRenderer failed: " << SDL_GetError() << "\n"; return false; }
    texture_ = SDL_CreateTexture(renderer_, SDL_PIXELFORMAT_RGBA8888,
        SDL_TEXTUREACCESS_STREAMING, DISPLAY_WIDTH, DISPLAY_HEIGHT);
    if (!texture_) { std::cerr << "SDL_CreateTexture failed: " << SDL_GetError() << "\n"; return false; }
    return true;
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
    if (audio_device_ > 0) {
        SDL_PauseAudioDevice(audio_device_, 0);
    } else {
        std::cerr << "Warning: Could not open audio device: " << SDL_GetError() << "\n";
    }
    return true;  // Audio failure is not fatal
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
    // Escape = toggle pause
    if (pressed && event.keysym.sym == SDLK_ESCAPE) {
        paused_ = !paused_;
        emulator_->set_paused(paused_);
        return;
    }
    // F5 = debugger toggle
    if (pressed && event.keysym.sym == SDLK_F5 && imgui_debugger_ui_) {
        if (imgui_debugger_ui_->is_visible()) imgui_debugger_ui_->hide();
        else imgui_debugger_ui_->show();
        return;
    }
    // Translate SDL2 scancode to MO5 key.
    // Mapping strategy: character-based. Press M on PC → get M on MO5.
    // The MO5 ROM's internal character table handles the rest.
    // Letters use AZERTY↔QWERTY swap (A↔Q, Z↔W) since the MO5 is AZERTY.
    // Punctuation maps by closest character match.
    MO5Key mo5_key;
    switch (event.keysym.scancode) {
        // Top letter row: QWERTY Q-W-E-R-T-Y -> MO5 A-Z-E-R-T-Y
        case SDL_SCANCODE_Q: mo5_key = MO5Key::A; break;
        case SDL_SCANCODE_W: mo5_key = MO5Key::Z; break;
        case SDL_SCANCODE_E: mo5_key = MO5Key::E; break;
        case SDL_SCANCODE_R: mo5_key = MO5Key::R; break;
        case SDL_SCANCODE_T: mo5_key = MO5Key::T; break;
        case SDL_SCANCODE_Y: mo5_key = MO5Key::Y; break;
        case SDL_SCANCODE_U: mo5_key = MO5Key::U; break;
        case SDL_SCANCODE_I: mo5_key = MO5Key::I; break;
        case SDL_SCANCODE_O: mo5_key = MO5Key::O; break;
        case SDL_SCANCODE_P: mo5_key = MO5Key::P; break;
        // Home row: QWERTY A-S-D-F-G-H-J-K-L -> MO5 Q-S-D-F-G-H-J-K-L
        case SDL_SCANCODE_A: mo5_key = MO5Key::Q; break;
        case SDL_SCANCODE_S: mo5_key = MO5Key::S; break;
        case SDL_SCANCODE_D: mo5_key = MO5Key::D; break;
        case SDL_SCANCODE_F: mo5_key = MO5Key::F; break;
        case SDL_SCANCODE_G: mo5_key = MO5Key::G; break;
        case SDL_SCANCODE_H: mo5_key = MO5Key::H; break;
        case SDL_SCANCODE_J: mo5_key = MO5Key::J; break;
        case SDL_SCANCODE_K: mo5_key = MO5Key::K; break;
        case SDL_SCANCODE_L: mo5_key = MO5Key::L; break;
        // Bottom row: QWERTY Z-X-C-V-B-N -> MO5 W-X-C-V-B-N
        case SDL_SCANCODE_Z: mo5_key = MO5Key::W; break;
        case SDL_SCANCODE_X: mo5_key = MO5Key::X; break;
        case SDL_SCANCODE_C: mo5_key = MO5Key::C; break;
        case SDL_SCANCODE_V: mo5_key = MO5Key::V; break;
        case SDL_SCANCODE_B: mo5_key = MO5Key::B; break;
        case SDL_SCANCODE_N: mo5_key = MO5Key::N; break;
        case SDL_SCANCODE_M: mo5_key = MO5Key::SLASH; break;         // MO5 scancode 0x1A produces 'M'
        // Punctuation — mapped by character, not physical position.
        // MO5Key names are confusing: they're named after AZERTY labels,
        // not the characters they produce. Comments show the actual output.
        case SDL_SCANCODE_COMMA:  mo5_key = MO5Key::M; break;        // MO5 scancode 0x08 produces ','
        case SDL_SCANCODE_PERIOD: mo5_key = MO5Key::COMMA; break;    // MO5 scancode 0x10 produces '.'
        case SDL_SCANCODE_SEMICOLON: mo5_key = MO5Key::PLUS; break;  // MO5 scancode 0x2E produces ';' (via SHIFT)
        case SDL_SCANCODE_SLASH:  mo5_key = MO5Key::SLASH; break;    // MO5 scancode 0x1A: SHIFT produces '/'
        case SDL_SCANCODE_LEFTBRACKET: mo5_key = MO5Key::STAR; break; // MO5 scancode 0x24 produces '*'
        case SDL_SCANCODE_RIGHTBRACKET: mo5_key = MO5Key::AT; break;  // MO5 scancode 0x18 produces '@'
        // Digit row
        case SDL_SCANCODE_0: mo5_key = MO5Key::Key0; break;
        case SDL_SCANCODE_1: mo5_key = MO5Key::Key1; break;
        case SDL_SCANCODE_2: mo5_key = MO5Key::Key2; break;
        case SDL_SCANCODE_3: mo5_key = MO5Key::Key3; break;
        case SDL_SCANCODE_4: mo5_key = MO5Key::Key4; break;
        case SDL_SCANCODE_5: mo5_key = MO5Key::Key5; break;
        case SDL_SCANCODE_6: mo5_key = MO5Key::Key6; break;
        case SDL_SCANCODE_7: mo5_key = MO5Key::Key7; break;
        case SDL_SCANCODE_8: mo5_key = MO5Key::Key8; break;
        case SDL_SCANCODE_9: mo5_key = MO5Key::Key9; break;
        // Special keys
        case SDL_SCANCODE_SPACE:     mo5_key = MO5Key::SPACE; break;
        case SDL_SCANCODE_RETURN:    mo5_key = MO5Key::ENTER; break;
        case SDL_SCANCODE_BACKSPACE: mo5_key = MO5Key::ACC2; break;
        case SDL_SCANCODE_DELETE:    mo5_key = MO5Key::EFF; break;
        case SDL_SCANCODE_GRAVE:     mo5_key = MO5Key::ACC; break;    // MO5 ACC (accent) key — ` is accent on PC
        case SDL_SCANCODE_TAB:       mo5_key = MO5Key::STOP; break;
        case SDL_SCANCODE_MINUS:     mo5_key = MO5Key::MINUS; break;
        case SDL_SCANCODE_EQUALS:    mo5_key = MO5Key::PLUS; break;
        case SDL_SCANCODE_APOSTROPHE: mo5_key = MO5Key::Key2; break;  // SHIFT = "
        // Arrows
        case SDL_SCANCODE_UP:    mo5_key = MO5Key::UP; break;
        case SDL_SCANCODE_DOWN:  mo5_key = MO5Key::DOWN; break;
        case SDL_SCANCODE_RIGHT: mo5_key = MO5Key::RIGHT; break;
        case SDL_SCANCODE_LEFT:  mo5_key = MO5Key::LEFT; break;
        // Navigation
        case SDL_SCANCODE_INSERT: mo5_key = MO5Key::INS; break;
        case SDL_SCANCODE_HOME:   mo5_key = MO5Key::RAZ; break;
        // Modifiers
        case SDL_SCANCODE_LSHIFT: mo5_key = MO5Key::SHIFT; break;
        case SDL_SCANCODE_RSHIFT: mo5_key = MO5Key::BASIC; break;
        case SDL_SCANCODE_LCTRL:  mo5_key = MO5Key::CNT; break;
        case SDL_SCANCODE_RCTRL:  mo5_key = MO5Key::CNT; break;
        case SDL_SCANCODE_LALT:   mo5_key = MO5Key::DOT; break;
        case SDL_SCANCODE_RALT:   mo5_key = MO5Key::DOT; break;
        default: return; // Unmapped key
    }
    emulator_->get_input_handler().set_key_state(mo5_key, pressed);
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
    if (!self->emulator_) {
        memset(stream, 0, len);
        return;
    }
    int samples = len / sizeof(int16_t);
    self->emulator_->get_audio_buffer(reinterpret_cast<int16*>(stream), samples);
}

} // namespace crayon
