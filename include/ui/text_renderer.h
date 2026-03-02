#ifndef TEXT_RENDERER_H
#define TEXT_RENDERER_H

#include <string>
#include <SDL.h>

#ifdef HAVE_SDL_TTF
#include <SDL_ttf.h>
#endif

class TextRenderer {
public:
    enum class FontSize { Small, Medium, Large };
    enum class TextAlign { Left, Center, Right };

    TextRenderer(SDL_Renderer* renderer);
    ~TextRenderer();

    bool initialize();
    
    // Main rendering methods
    bool render_text(const std::string& text, int x, int y,
                     SDL_Color color, FontSize size = FontSize::Small,
                     TextAlign align = TextAlign::Left);
    bool render_text_with_shadow(const std::string& text, int x, int y,
                                  SDL_Color color, SDL_Color shadow_color,
                                  FontSize size = FontSize::Small,
                                  TextAlign align = TextAlign::Left);
    
    // Convenience overloads for compatibility
    bool render_text(SDL_Renderer* /*renderer*/, const char* text, int x, int y,
                     SDL_Color color, TextAlign align = TextAlign::Left) {
        return render_text(std::string(text), x, y, color, FontSize::Small, align);
    }
    bool render_text_with_shadow(SDL_Renderer* /*renderer*/, const char* text, int x, int y,
                                  SDL_Color color, TextAlign align = TextAlign::Left) {
        SDL_Color shadow = {0, 0, 0, static_cast<Uint8>(color.a / 2)};
        return render_text_with_shadow(std::string(text), x, y, color, shadow, FontSize::Small, align);
    }
    
    // Measurement methods
    bool measure_text(const std::string& text, FontSize size,
                      int* out_width, int* out_height);
    int measure_text_width(const std::string& text, FontSize size = FontSize::Small);
    int measure_text_width(const char* text) {
        return measure_text_width(std::string(text), FontSize::Small);
    }
    int get_font_height(FontSize size = FontSize::Small) const;
    
    bool has_ttf_support() const;

private:
    bool render_text_ttf(const std::string& text, int x, int y,
                         SDL_Color color, FontSize size, TextAlign align);
    bool render_text_fallback(const std::string& text, int x, int y,
                              SDL_Color color, FontSize size, TextAlign align);
    int get_font_size_pixels(FontSize size) const;
    int apply_alignment(int x, int text_width, TextAlign align) const;

    SDL_Renderer* renderer_;
#ifdef HAVE_SDL_TTF
    TTF_Font* font_small_ = nullptr;
    TTF_Font* font_medium_ = nullptr;
    TTF_Font* font_large_ = nullptr;
    bool ttf_initialized_ = false;
#endif
};

#endif // TEXT_RENDERER_H
