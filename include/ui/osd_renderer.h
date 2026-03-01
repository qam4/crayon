#ifndef OSD_RENDERER_H
#define OSD_RENDERER_H

#include <string>
#include <cstdint>
#include <SDL.h>
#include "ui/text_renderer.h"

class OSDRenderer {
public:
    enum class OSDPosition { TopLeft, TopRight, BottomLeft, BottomRight };
    enum class FontSize { Small, Medium, Large };

    OSDRenderer(SDL_Renderer* renderer);
    ~OSDRenderer();

    bool initialize();
    void render_fps(float fps, OSDPosition position);
    void render_notification(const std::string& message, OSDPosition position);
    void render_status_bar(const std::string& text);
    void set_font_size(FontSize size);
    void set_opacity(int opacity);
    void show_notification(const std::string& message, int duration_ms);
    void update(uint32_t current_time);

    FontSize get_font_size() const { return font_size_; }
    int get_opacity() const { return opacity_; }

private:
    SDL_Renderer* renderer_;
    TextRenderer text_renderer_;
    FontSize font_size_ = FontSize::Medium;
    int opacity_ = 100;
    std::string current_notification_;
    uint32_t notification_start_time_ = 0;
    int notification_duration_ = 0;
    bool notification_active_ = false;
    static constexpr int PADDING = 10;
};

#endif // OSD_RENDERER_H
