#ifndef OSD_RENDERER_H
#define OSD_RENDERER_H

#include <string>
#include <cstdint>
#include <SDL.h>

class TextRenderer;
class ConfigManager;

class OSDRenderer {
public:
    enum class OSDPosition { TopLeft, TopRight, BottomLeft, BottomRight };

    OSDRenderer(SDL_Renderer* renderer, TextRenderer* text_renderer, ConfigManager* config);
    ~OSDRenderer();

    void render_fps(float fps);
    void render_notification();
    void render_status_bar(const std::string& text);
    void set_opacity(uint8_t opacity);
    void show_notification(const std::string& message, int duration_ms);
    void update(uint32_t current_time);
    
    bool is_fps_enabled() const;
    void set_fps_enabled(bool enabled);
    OSDPosition get_fps_position() const;
    void set_fps_position(OSDPosition position);
    uint8_t get_opacity() const { return opacity_; }

private:
    void get_position_coords(OSDPosition position, int text_width, int text_height, int& x, int& y);
    
    SDL_Renderer* renderer_;
    TextRenderer* text_renderer_;
    ConfigManager* config_;
    uint8_t opacity_ = 200;
    std::string current_notification_;
    uint32_t notification_start_time_ = 0;
    int notification_duration_ = 0;
    bool notification_active_ = false;
    static constexpr int PADDING = 10;
};

#endif // OSD_RENDERER_H
