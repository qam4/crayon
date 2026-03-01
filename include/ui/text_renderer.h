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

    TextRenderer(SDL_Renderer* renderer);
    ~TextRenderer();

    bool initialize();
    bool render_text(const std::string& text, int x, int y,
                     SDL_Color color, FontSize size = FontSize::Medium);
    bool measure_text(const std::string& text, FontSize size,
                      int* out_width, int* out_height);
    bool has_ttf_support() const;

private:
    bool render_text_ttf(const std::string& text, int x, int y,
                         SDL_Color color, FontSize size);
    bool render_text_fallback(const std::string& text, int x, int y,
                              SDL_Color color, FontSize size);
    int get_font_size_pixels(FontSize size) const;

    SDL_Renderer* renderer_;
#ifdef HAVE_SDL_TTF
    TTF_Font* font_small_ = nullptr;
    TTF_Font* font_medium_ = nullptr;
    TTF_Font* font_large_ = nullptr;
    bool ttf_initialized_ = false;
#endif
};

#endif // TEXT_RENDERER_H
