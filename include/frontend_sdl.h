#ifndef CRAYON_FRONTEND_SDL_H
#define CRAYON_FRONTEND_SDL_H

#include "frontend.h"
#include "emulator_core.h"
#include "debugger.h"
#include "debugger_ui.h"
#include "ui/osd_renderer.h"
#include "ui/imgui_debugger_ui.h"
#include <SDL.h>
#include <memory>
#include <vector>

class TextRenderer;
class MenuSystem;
class ConfigManager;
class FileBrowser;
class ZIPHandler;
class MessageDialog;
class ProgressDialog;
class RecentFilesList;
class SaveStateManagerUI;

namespace crayon {

class SDLFrontend : public Frontend {
public:
    SDLFrontend();
    ~SDLFrontend() override;

    bool initialize(const FrontendConfig& config) override;
    void shutdown() override;
    void run() override;
    bool is_running() const override;

    void render_frame() override;
    void process_audio() override;
    void process_input() override;

    MenuAction process_menu() override;
    void show_message(const std::string& message) override;

    EmulatorCore* get_emulator() override;

    void save_screenshot(const std::string& filename) override;
    void dump_framebuffer(const std::string& filename) override;

    void set_frame_limit(int frames) { frame_limit_ = frames; }

private:
    SDL_Window* window_ = nullptr;
    SDL_Renderer* renderer_ = nullptr;
    SDL_Texture* texture_ = nullptr;
    SDL_AudioDeviceID audio_device_ = 0;

    std::unique_ptr<EmulatorCore> emulator_;
    std::unique_ptr<Debugger> debugger_;
    std::unique_ptr<DebuggerUI> debugger_ui_;
    std::unique_ptr<ImGuiDebuggerUI> imgui_debugger_ui_;

    std::unique_ptr<TextRenderer> text_renderer_;
    std::unique_ptr<MenuSystem> menu_system_;
    std::unique_ptr<ConfigManager> config_manager_;
    std::unique_ptr<FileBrowser> file_browser_;
    std::unique_ptr<ZIPHandler> zip_handler_;
    std::unique_ptr<MessageDialog> message_dialog_;
    std::unique_ptr<ProgressDialog> progress_dialog_;
    std::unique_ptr<RecentFilesList> recent_cartridges_;
    std::unique_ptr<RecentFilesList> recent_roms_;
    std::unique_ptr<SaveStateManagerUI> save_state_manager_;
    std::unique_ptr<OSDRenderer> osd_renderer_;

    FrontendConfig config_;
    bool running_ = false;
    bool paused_ = false;
    uint32_t frame_count_ = 0;
    uint32_t frame_limit_ = 0;
    float current_fps_ = 0.0f;

    bool init_video();
    bool init_audio();
    void cleanup_video();
    void cleanup_audio();
    void handle_keyboard_event(const SDL_KeyboardEvent& event);
    void handle_menu_action(MenuAction action);

    static void audio_callback(void* userdata, uint8_t* stream, int len);
};

} // namespace crayon

#endif // CRAYON_FRONTEND_SDL_H
