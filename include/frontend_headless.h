#ifndef CRAYON_FRONTEND_HEADLESS_H
#define CRAYON_FRONTEND_HEADLESS_H

#include "frontend.h"
#include "emulator_core.h"
#include "debugger.h"
#include "debugger_ui.h"
#include <memory>
#include <chrono>
#include <vector>

namespace crayon {

class HeadlessFrontend : public Frontend {
public:
    HeadlessFrontend();
    ~HeadlessFrontend() override;

    bool initialize(const FrontendConfig& config) override;
    void shutdown() override;
    void run() override;
    bool is_running() const override { return running_; }

    void render_frame() override;
    void process_audio() override;
    void process_input() override;
    MenuAction process_menu() override;
    void show_message(const std::string& message) override;

    EmulatorCore* get_emulator() override { return emulator_.get(); }

    void save_screenshot(const std::string& filename) override;
    void dump_framebuffer(const std::string& filename) override;

    void set_frame_limit(int frames) { frame_limit_ = frames; }
    void set_trace_path(const std::string& path) { trace_path_ = path; }
    void set_type_string(const std::string& text, int delay_frames) {
        type_text_ = text;
        type_delay_frames_ = delay_frames;
    }

private:
    FrontendConfig config_;
    std::unique_ptr<EmulatorCore> emulator_;
    std::unique_ptr<Debugger> debugger_;
    std::unique_ptr<DebuggerUI> debugger_ui_;

    bool running_ = false;
    int frame_count_ = 0;
    int frame_limit_ = 0;
    std::string trace_path_;
    std::string type_text_;
    int type_delay_frames_ = 60;
    size_t type_index_ = 0;
    int type_key_hold_frames_ = 0;  // frames remaining to hold current key
};

} // namespace crayon

#endif // CRAYON_FRONTEND_HEADLESS_H
