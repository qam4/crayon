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
    
    // Try to load a system font
    // Common font paths on different systems
    const char* font_paths[] = {
        // Windows
        "C:/Windows/Fonts/arial.ttf",
        "C:/Windows/Fonts/consola.ttf",
        // Linux
        "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf",
        "/usr/share/fonts/TTF/DejaVuSans.ttf",
        // macOS
        "/System/Library/Fonts/Helvetica.ttc",
        "/Library/Fonts/Arial.ttf",
        nullptr
    };
    
    // Try to load fonts from system paths
    for (const char** path = font_paths; *path != nullptr; ++path) {
        if (!font_small_) {
            font_small_ = TTF_OpenFont(*path, 10);
        }
        if (!font_medium_) {
            font_medium_ = TTF_OpenFont(*path, 13);
        }
        if (!font_large_) {
            font_large_ = TTF_OpenFont(*path, 18);
        }
        
        // If we loaded all fonts, we're done
        if (font_small_ && font_medium_ && font_large_) {
            break;
        }
    }
    
    // If TTF fonts failed to load, we'll fall back to the simple renderer
    if (!font_small_ && !font_medium_ && !font_large_) {
        ttf_initialized_ = false;
    }
#endif
    return true;
}

bool TextRenderer::render_text(const std::string& text, int x, int y,
                                SDL_Color color, FontSize size, TextAlign align) {
#ifdef HAVE_SDL_TTF
    if (ttf_initialized_) return render_text_ttf(text, x, y, color, size, align);
#endif
    return render_text_fallback(text, x, y, color, size, align);
}

bool TextRenderer::render_text_with_shadow(const std::string& text, int x, int y,
                                            SDL_Color color, SDL_Color shadow_color,
                                            FontSize size, TextAlign align) {
    // Render shadow first (offset by 1-2 pixels)
    render_text(text, x + 1, y + 1, shadow_color, size, align);
    // Render main text on top
    return render_text(text, x, y, color, size, align);
}

bool TextRenderer::measure_text(const std::string& text, FontSize size,
                                 int* out_width, int* out_height) {
#ifdef HAVE_SDL_TTF
    if (ttf_initialized_) {
        TTF_Font* font = nullptr;
        switch (size) {
            case FontSize::Small: font = font_small_; break;
            case FontSize::Medium: font = font_medium_; break;
            case FontSize::Large: font = font_large_; break;
        }
        if (font) {
            TTF_SizeText(font, text.c_str(), out_width, out_height);
            return true;
        }
    }
#endif
    // Fallback measurement
    int px = get_font_size_pixels(size);
    *out_width = static_cast<int>(text.length()) * (px / 2);
    *out_height = px;
    return true;
}

int TextRenderer::measure_text_width(const std::string& text, FontSize size) {
    int width = 0, height = 0;
    measure_text(text, size, &width, &height);
    return width;
}

int TextRenderer::get_font_height(FontSize size) const {
    return get_font_size_pixels(size);
}

bool TextRenderer::has_ttf_support() const {
#ifdef HAVE_SDL_TTF
    return ttf_initialized_;
#else
    return false;
#endif
}

bool TextRenderer::render_text_ttf(const std::string& text, int x, int y,
                                    SDL_Color color, FontSize size, TextAlign align) {
#ifdef HAVE_SDL_TTF
    TTF_Font* font = nullptr;
    switch (size) {
        case FontSize::Small: font = font_small_; break;
        case FontSize::Medium: font = font_medium_; break;
        case FontSize::Large: font = font_large_; break;
    }
    
    if (!font) return false;
    
    SDL_Surface* surface = TTF_RenderText_Blended(font, text.c_str(), color);
    if (!surface) return false;
    
    SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer_, surface);
    if (!texture) {
        SDL_FreeSurface(surface);
        return false;
    }
    
    int text_width = surface->w;
    int adjusted_x = apply_alignment(x, text_width, align);
    
    SDL_Rect dest = { adjusted_x, y, surface->w, surface->h };
    SDL_RenderCopy(renderer_, texture, nullptr, &dest);
    
    SDL_DestroyTexture(texture);
    SDL_FreeSurface(surface);
    return true;
#else
    return false;
#endif
}

bool TextRenderer::render_text_fallback(const std::string& text, int x, int y,
                                        SDL_Color color, FontSize size, TextAlign align) {
    // Simple fallback: render each character as a small rectangle
    // This is a minimal implementation for when SDL_TTF is not available
    int px = get_font_size_pixels(size);
    int char_width = px / 2;
    int text_width = static_cast<int>(text.length()) * char_width;
    int adjusted_x = apply_alignment(x, text_width, align);
    
    SDL_SetRenderDrawColor(renderer_, color.r, color.g, color.b, color.a);
    
    for (size_t i = 0; i < text.length(); ++i) {
        SDL_Rect rect = {
            adjusted_x + static_cast<int>(i) * char_width,
            y,
            char_width - 1,
            px
        };
        SDL_RenderDrawRect(renderer_, &rect);
    }
    
    return true;
}

int TextRenderer::get_font_size_pixels(FontSize size) const {
    switch (size) {
        case FontSize::Small: return 10;
        case FontSize::Medium: return 13;
        case FontSize::Large: return 18;
    }
    return 16;
}

int TextRenderer::apply_alignment(int x, int text_width, TextAlign align) const {
    switch (align) {
        case TextAlign::Left:
            return x;
        case TextAlign::Center:
            return x - (text_width / 2);
        case TextAlign::Right:
            return x - text_width;
    }
    return x;
}
