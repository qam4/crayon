#include "ui/osd_renderer.h"
#include <sstream>
#include <iomanip>

OSDRenderer::OSDRenderer(SDL_Renderer* renderer)
    : renderer_(renderer), text_renderer_(renderer) {}

OSDRenderer::~OSDRenderer() = default;

bool OSDRenderer::initialize() { return text_renderer_.initialize(); }

void OSDRenderer::render_fps(float fps, OSDPosition /*position*/) {
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(1) << fps << " FPS";
    SDL_Color white = {255, 255, 255, 255};
    text_renderer_.render_text(oss.str(), PADDING, PADDING, white);
}

void OSDRenderer::render_notification(const std::string& message, OSDPosition /*position*/) {
    SDL_Color yellow = {255, 255, 0, 255};
    text_renderer_.render_text(message, PADDING, PADDING + 20, yellow);
}

void OSDRenderer::render_status_bar(const std::string& text) {
    SDL_Color white = {255, 255, 255, 255};
    int w, h;
    SDL_GetRendererOutputSize(renderer_, &w, &h);
    text_renderer_.render_text(text, PADDING, h - 20, white);
}

void OSDRenderer::set_font_size(FontSize size) { font_size_ = size; }
void OSDRenderer::set_opacity(int opacity) { opacity_ = opacity; }

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
