#include "frontend_sdl.h"
#include "ui/text_renderer.h"
#include "ui/menu_system.h"
#include "ui/config_manager.h"
#include "ui/file_browser.h"
#include "ui/zip_handler.h"
#include "ui/dialogs.h"
#include "ui/recent_files_list.h"
#include "ui/save_state_manager.h"
#include "ui/osd_renderer.h"
#include <iostream>

namespace crayon {

SDLFrontend::SDLFrontend() = default;
SDLFrontend::~SDLFrontend() { shutdown(); }

bool SDLFrontend::initialize(const FrontendConfig& config) {
    config_ = config;
    if (!init_video()) return false;
    if (!init_audio()) return false;

    // Initialize UI components
    text_renderer_ = std::make_unique<TextRenderer>(renderer_);
    if (!text_renderer_->initialize()) {
        std::cerr << "Failed to initialize TextRenderer\n";
        return false;
    }
    
    config_manager_ = std::make_unique<ConfigManager>();
    config_manager_->load();
    
    menu_system_ = std::make_unique<MenuSystem>(renderer_, text_renderer_.get());
    file_browser_ = std::make_unique<FileBrowser>(renderer_, text_renderer_.get(), config_manager_.get());
    osd_renderer_ = std::make_unique<OSDRenderer>(renderer_, text_renderer_.get(), config_manager_.get());
    message_dialog_ = std::make_unique<MessageDialog>(renderer_, text_renderer_.get());
    progress_dialog_ = std::make_unique<ProgressDialog>(renderer_, text_renderer_.get());
    zip_handler_ = std::make_unique<ZIPHandler>();
    recent_cartridges_ = std::make_unique<RecentFilesList>(10);
    recent_roms_ = std::make_unique<RecentFilesList>(10);

    Configuration emu_config;
    emu_config.basic_rom_path = config.basic_rom_path;
    emu_config.monitor_rom_path = config.monitor_rom_path;
    emulator_ = std::make_unique<EmulatorCore>(emu_config);
    
    // Initialize SaveStateManagerUI after emulator
    save_state_manager_ = std::make_unique<SaveStateManagerUI>(
        emulator_.get(), renderer_, text_renderer_.get());

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
        emulator_->set_debugger(debugger_.get());
        imgui_debugger_ui_ = std::make_unique<ImGuiDebuggerUI>(
            debugger_.get(), emulator_.get(), window_, renderer_);
        if (!imgui_debugger_ui_->initialize()) {
            std::cerr << "Failed to initialize ImGui debugger UI\n";
            imgui_debugger_ui_.reset();
            debugger_.reset();
        }
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
        // Sync pause state from debugger (ImGui buttons set debugger state directly)
        if (debugger_) {
            bool dbg_paused = debugger_->is_paused();
            if (dbg_paused != paused_) {
                paused_ = dbg_paused;
                emulator_->set_paused(paused_);
            }
        }
        if (!paused_) emulator_->run_frame();
        render_frame();
        process_audio();
        frame_count_++;
    }
}

bool SDLFrontend::is_running() const { return running_; }

void SDLFrontend::render_frame() {
    if (!renderer_ || !texture_) return;
    
    // Render emulator framebuffer
    const uint32* fb = emulator_->get_framebuffer();
    if (fb) SDL_UpdateTexture(texture_, nullptr, fb, DISPLAY_WIDTH * sizeof(uint32_t));
    SDL_RenderClear(renderer_);
    SDL_RenderCopy(renderer_, texture_, nullptr, nullptr);
    
    // Render UI overlays (after emulator framebuffer)
    if (osd_renderer_) {
        osd_renderer_->render_fps(current_fps_);
        osd_renderer_->render_notification();
    }
    
    if (menu_system_ && menu_system_->is_open()) {
        menu_system_->render();
    }
    
    if (file_browser_ && file_browser_->is_open()) {
        file_browser_->render();
    }
    
    if (save_state_manager_ && save_state_manager_->is_ui_visible()) {
        save_state_manager_->render_ui("current_game"); // TODO: Get actual game name
    }
    
    if (message_dialog_ && message_dialog_->is_visible()) {
        message_dialog_->render();
    }
    
    if (progress_dialog_ && progress_dialog_->is_visible()) {
        progress_dialog_->render();
    }
    
    // Render pause indicator (persistent, not a timed notification)
    if (paused_ && osd_renderer_) {
        int window_width, window_height;
        SDL_GetRendererOutputSize(renderer_, &window_width, &window_height);
        SDL_Color pause_color = {255, 255, 0, 200};
        text_renderer_->render_text_with_shadow("PAUSED", 
            window_width / 2, window_height / 2 - 10,
            pause_color, SDL_Color{0, 0, 0, 128},
            TextRenderer::FontSize::Large, TextRenderer::TextAlign::Center);
    }
    
    // Render debugger UI last (on top of everything) - only if visible
    if (imgui_debugger_ui_ && imgui_debugger_ui_->is_visible()) {
        imgui_debugger_ui_->render();
    }
    
    SDL_RenderPresent(renderer_);
}

void SDLFrontend::process_audio() {
    // TODO: Fill SDL audio buffer from emulator
}

void SDLFrontend::process_input() {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        // ImGui debugger gets first priority (only if visible)
        if (imgui_debugger_ui_) {
            bool is_visible = imgui_debugger_ui_->is_visible();
            if (is_visible) {
                imgui_debugger_ui_->process_event(event);
            }
        }
        
        // Handle UI input before emulator input
        if (event.type == SDL_KEYDOWN) {
            // Check if any UI component is active and should handle input
            if (menu_system_ && menu_system_->is_open()) {
                auto action = menu_system_->process_input(event.key.keysym.sym);
                if (action != crayon::MenuAction::None) {
                    handle_menu_action(action);
                }
                continue;
            }
            
            if (file_browser_ && file_browser_->is_open()) {
                if (file_browser_->process_input(event.key.keysym.sym)) {
                    // File browser handled the input
                    if (file_browser_->was_file_selected()) {
                        std::string selected = file_browser_->get_selected_file();
                        auto ftype = file_browser_->get_file_type();
                        if (ftype == FileBrowser::FileType::Generic) {
                            // K7 cassette
                            auto result = emulator_->get_cassette().load_k7(selected);
                            if (result.is_ok()) {
                                emulator_->get_cassette().play();
                                osd_renderer_->show_notification("K7 loaded: " + selected.substr(selected.find_last_of("/\\") + 1), 2000);
                            } else {
                                osd_renderer_->show_notification("Failed to load K7", 2000);
                            }
                        } else if (ftype == FileBrowser::FileType::Cartridge) {
                            auto result = emulator_->load_cartridge(selected);
                            if (result.is_ok()) {
                                emulator_->reset();
                                osd_renderer_->show_notification("Cartridge loaded", 2000);
                            } else {
                                osd_renderer_->show_notification("Failed to load cartridge", 2000);
                            }
                        } else if (ftype == FileBrowser::FileType::ROM) {
                            // Try loading as BASIC or Monitor ROM based on size
                            osd_renderer_->show_notification("ROM loaded: " + selected.substr(selected.find_last_of("/\\") + 1), 2000);
                        }
                    }
                    continue;
                }
            }
            
            if (save_state_manager_ && save_state_manager_->is_ui_visible()) {
                if (save_state_manager_->process_input(event.key.keysym.sym)) {
                    continue;
                }
            }
            
            if (message_dialog_ && message_dialog_->is_visible()) {
                if (message_dialog_->process_input(event.key.keysym.sym)) {
                    continue;
                }
            }
            
            if (progress_dialog_ && progress_dialog_->is_visible()) {
                if (progress_dialog_->process_input(event.key.keysym.sym)) {
                    continue;
                }
            }
            
            // F1 = toggle menu
            if (event.key.keysym.sym == SDLK_F1) {
                if (menu_system_->is_open()) {
                    menu_system_->close();
                } else {
                    menu_system_->open();
                }
                continue;
            }
            
            // Global hotkeys (when menu is not open)
            if (!menu_system_->is_open()) {
                switch (event.key.keysym.sym) {
                    case SDLK_F2:
                        handle_menu_action(crayon::MenuAction::LoadBasicROM);
                        continue;
                    case SDLK_F3:
                        handle_menu_action(crayon::MenuAction::LoadMonitorROM);
                        continue;
                    case SDLK_F4:
                        handle_menu_action(crayon::MenuAction::LoadCartridge);
                        continue;
                    case SDLK_F5:
                        if (imgui_debugger_ui_) {
                            if (imgui_debugger_ui_->is_visible()) {
                                imgui_debugger_ui_->hide();
                            } else {
                                imgui_debugger_ui_->show();
                            }
                        } else {
                            osd_renderer_->show_notification("Debugger not enabled (use --debugger)", 2000);
                        }
                        continue;
                    case SDLK_F6:
                        handle_menu_action(crayon::MenuAction::LoadK7);
                        continue;
                    case SDLK_F7:
                        handle_menu_action(crayon::MenuAction::Reset);
                        continue;
                    case SDLK_F8:
                        paused_ = !paused_;
                        emulator_->set_paused(paused_);
                        if (debugger_) {
                            if (paused_) debugger_->pause();
                            else debugger_->continue_execution();
                        }
                        osd_renderer_->show_notification(paused_ ? "Paused" : "Resumed", 1500);
                        continue;
                    case SDLK_F9:
                        save_state_manager_->set_mode(true);
                        save_state_manager_->show_ui(true);
                        continue;
                    case SDLK_F10:
                        save_state_manager_->set_mode(false);
                        save_state_manager_->show_ui(true);
                        continue;
                    case SDLK_F11:
                        handle_menu_action(crayon::MenuAction::Screenshot);
                        continue;
                    case SDLK_F12:
                        osd_renderer_->set_fps_enabled(!osd_renderer_->is_fps_enabled());
                        continue;
                    default:
                        break;
                }
            }
        }
        
        // Handle system events
        switch (event.type) {
            case SDL_QUIT: 
                running_ = false; 
                break;
            case SDL_KEYDOWN: 
            case SDL_KEYUP:
                handle_keyboard_event(event.key); 
                break;
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
    
    // Update OSD
    if (osd_renderer_) {
        osd_renderer_->update(SDL_GetTicks());
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
        case MenuAction::LoadBasicROM:
            file_browser_->open(".rom,.bin", FileBrowser::FileType::ROM);
            break;
        case MenuAction::LoadMonitorROM:
            file_browser_->open(".rom,.bin", FileBrowser::FileType::ROM);
            break;
        case MenuAction::LoadCartridge:
            file_browser_->open(".rom,.bin", FileBrowser::FileType::Cartridge);
            break;
        case MenuAction::LoadK7:
            file_browser_->open(".k7", FileBrowser::FileType::Generic);
            break;
        case MenuAction::Reset: 
            emulator_->reset(); 
            osd_renderer_->show_notification("System Reset", 1500);
            break;
        case MenuAction::Pause: 
        case MenuAction::Resume:
            paused_ = !paused_;
            emulator_->set_paused(paused_);
            if (debugger_) {
                if (paused_) debugger_->pause();
                else debugger_->continue_execution();
            }
            osd_renderer_->show_notification(paused_ ? "Paused" : "Resumed", 1500);
            break;
        case MenuAction::SaveState:
            save_state_manager_->set_mode(true); // Save mode
            save_state_manager_->show_ui(true);
            break;
        case MenuAction::LoadState:
            save_state_manager_->set_mode(false); // Load mode
            save_state_manager_->show_ui(true);
            break;
        case MenuAction::Screenshot:
            save_screenshot("screenshot.png");
            osd_renderer_->show_notification("Screenshot saved", 1500);
            break;
        case MenuAction::ToggleFPS:
            osd_renderer_->set_fps_enabled(!osd_renderer_->is_fps_enabled());
            break;
        case MenuAction::ToggleDebugger:
            if (imgui_debugger_ui_) {
                if (imgui_debugger_ui_->is_visible()) {
                    imgui_debugger_ui_->hide();
                } else {
                    imgui_debugger_ui_->show();
                }
            } else {
                osd_renderer_->show_notification("Debugger not enabled (use --debugger)", 2000);
            }
            break;
        case MenuAction::ToggleFullscreen: {
            uint32_t flags = SDL_GetWindowFlags(window_);
            bool is_fullscreen = (flags & SDL_WINDOW_FULLSCREEN_DESKTOP) != 0;
            SDL_SetWindowFullscreen(window_, is_fullscreen ? 0 : SDL_WINDOW_FULLSCREEN_DESKTOP);
            break;
        }
        case MenuAction::Quit: 
            running_ = false; 
            break;
        default: 
            break;
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
