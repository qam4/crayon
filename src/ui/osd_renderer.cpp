#include "ui/osd_renderer.h"
#include "ui/text_renderer.h"
#include "ui/config_manager.h"
#include <sstream>
#include <iomanip>

OSDRenderer::OSDRenderer(SDL_Renderer* renderer, TextRenderer* text_renderer, ConfigManager* config)
    : renderer_(renderer), text_renderer_(text_renderer), config_(config) {}

OSDRenderer::~OSDRenderer() = default;

void OSDRenderer::render_fps(float fps) {
    if (!is_fps_enabled()) return;
    
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(1) << fps << " FPS";
    std::string fps_text = oss.str();
    
    // Measure text dimensions
    int text_width = text_renderer_->measure_text_width(fps_text.c_str());
    int text_height = text_renderer_->get_font_height();
    
    // Get position coordinates
    int x, y;
    get_position_coords(get_fps_position(), text_width, text_height, x, y);
    
    // Apply opacity
    SDL_Color color = {255, 255, 255, opacity_};
    text_renderer_->render_text(renderer_, fps_text.c_str(), x, y, color, TextRenderer::TextAlign::Left);
}

void OSDRenderer::render_notification() {
    if (!notification_active_) return;
    
    // Measure text dimensions
    int text_width = text_renderer_->measure_text_width(current_notification_.c_str());
    int text_height = text_renderer_->get_font_height();
    
    // Center notification at top of screen
    int window_width, window_height;
    SDL_GetRendererOutputSize(renderer_, &window_width, &window_height);
    int x = (window_width - text_width) / 2;
    int y = PADDING;
    
    // Semi-transparent background
    SDL_SetRenderDrawBlendMode(renderer_, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(renderer_, 0, 0, 0, opacity_ / 2);
    SDL_Rect bg = {x - 5, y - 2, text_width + 10, text_height + 4};
    SDL_RenderFillRect(renderer_, &bg);
    
    // Render notification text with shadow
    SDL_Color color = {255, 255, 0, opacity_};
    text_renderer_->render_text_with_shadow(renderer_, current_notification_.c_str(), x, y, color, TextRenderer::TextAlign::Left);
}

void OSDRenderer::render_status_bar(const std::string& text) {
    if (text.empty()) return;
    
    int window_width, window_height;
    SDL_GetRendererOutputSize(renderer_, &window_width, &window_height);
    
    // Measure text dimensions
    int text_width = text_renderer_->measure_text_width(text.c_str());
    int text_height = text_renderer_->get_font_height();
    
    // Position at bottom left
    int x = PADDING;
    int y = window_height - text_height - PADDING;
    
    // Semi-transparent background
    SDL_SetRenderDrawBlendMode(renderer_, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(renderer_, 0, 0, 0, opacity_ / 2);
    SDL_Rect bg = {x - 5, y - 2, text_width + 10, text_height + 4};
    SDL_RenderFillRect(renderer_, &bg);
    
    // Apply opacity
    SDL_Color color = {200, 200, 200, opacity_};
    text_renderer_->render_text(renderer_, text.c_str(), x, y, color, TextRenderer::TextAlign::Left);
}

void OSDRenderer::set_opacity(uint8_t opacity) {
    opacity_ = opacity;
}

void OSDRenderer::show_notification(const std::string& message, int duration_ms) {
    current_notification_ = message;
    notification_duration_ = duration_ms;
    notification_start_time_ = SDL_GetTicks();
    notification_active_ = true;
}

void OSDRenderer::update(uint32_t current_time) {
    if (notification_active_ &&
        (current_time - notification_start_time_) > static_cast<uint32_t>(notification_duration_)) {
        notification_active_ = false;
    }
}

bool OSDRenderer::is_fps_enabled() const {
    return config_->get_fps_display_enabled();
}

void OSDRenderer::set_fps_enabled(bool enabled) {
    config_->set_fps_display_enabled(enabled);
    config_->save();
}

OSDRenderer::OSDPosition OSDRenderer::get_fps_position() const {
    std::string pos = config_->get_fps_position();
    if (pos == "top-right") return OSDPosition::TopRight;
    if (pos == "bottom-left") return OSDPosition::BottomLeft;
    if (pos == "bottom-right") return OSDPosition::BottomRight;
    return OSDPosition::TopLeft; // default
}

void OSDRenderer::set_fps_position(OSDPosition position) {
    std::string pos_str;
    switch (position) {
        case OSDPosition::TopLeft: pos_str = "top-left"; break;
        case OSDPosition::TopRight: pos_str = "top-right"; break;
        case OSDPosition::BottomLeft: pos_str = "bottom-left"; break;
        case OSDPosition::BottomRight: pos_str = "bottom-right"; break;
    }
    config_->set_fps_position(pos_str);
    config_->save();
}

void OSDRenderer::get_position_coords(OSDPosition position, int text_width, int text_height, int& x, int& y) {
    int window_width, window_height;
    SDL_GetRendererOutputSize(renderer_, &window_width, &window_height);
    
    switch (position) {
        case OSDPosition::TopLeft:
            x = PADDING;
            y = PADDING;
            break;
        case OSDPosition::TopRight:
            x = window_width - text_width - PADDING;
            y = PADDING;
            break;
        case OSDPosition::BottomLeft:
            x = PADDING;
            y = window_height - text_height - PADDING;
            break;
        case OSDPosition::BottomRight:
            x = window_width - text_width - PADDING;
            y = window_height - text_height - PADDING;
            break;
    }
}
