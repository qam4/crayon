#include "ui/text_renderer.h"
#include <cstring>

TextRenderer::TextRenderer(SDL_Renderer* renderer) : renderer_(renderer) {}

TextRenderer::~TextRenderer() {
#ifdef HAVE_SDL_TTF
    if (font_small_) TTF_CloseFont(font_small_);
    if (font_medium_) TTF_CloseFont(font_medium_);
    if (font_large_) TTF_CloseFont(font_large_);
    if (ttf_initialized_) TTF_Quit();
#endif
}

bool TextRenderer::initialize() {
#ifdef HAVE_SDL_TTF
    if (TTF_Init() < 0) return false;
    ttf_initialized_ = true;
    // TODO: Load fonts
#endif
    return true;
}

bool TextRenderer::render_text(const std::string& text, int x, int y,
                                SDL_Color color, FontSize size) {
#ifdef HAVE_SDL_TTF
    if (ttf_initialized_) return render_text_ttf(text, x, y, color, size);
#endif
    return render_text_fallback(text, x, y, color, size);
}

bool TextRenderer::measure_text(const std::string& text, FontSize size,
                                 int* out_width, int* out_height) {
    int px = get_font_size_pixels(size);
    *out_width = static_cast<int>(text.length()) * (px / 2);
    *out_height = px;
    return true;
}

bool TextRenderer::has_ttf_support() const {
#ifdef HAVE_SDL_TTF
    return ttf_initialized_;
#else
    return false;
#endif
}

bool TextRenderer::render_text_ttf(const std::string&, int, int, SDL_Color, FontSize) {
    // TODO: Implement TTF rendering
    return false;
}

bool TextRenderer::render_text_fallback(const std::string&, int, int, SDL_Color, FontSize) {
    // TODO: Implement basic bitmap font fallback
    return false;
}

int TextRenderer::get_font_size_pixels(FontSize size) const {
    switch (size) {
        case FontSize::Small: return 12;
        case FontSize::Medium: return 16;
        case FontSize::Large: return 24;
    }
    return 16;
}
