#include "ui/dialogs.h"
#include "ui/text_renderer.h"

// Base Dialog
Dialog::Dialog(SDL_Renderer* renderer, TextRenderer* text_renderer)
    : renderer_(renderer), text_renderer_(text_renderer) {}
Dialog::~Dialog() = default;
void Dialog::show() { visible_ = true; }
void Dialog::hide() { visible_ = false; }

void Dialog::render_overlay() {
    SDL_SetRenderDrawBlendMode(renderer_, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(renderer_, 0, 0, 0, 160);
    int w, h;
    SDL_GetRendererOutputSize(renderer_, &w, &h);
    SDL_Rect rect = {0, 0, w, h};
    SDL_RenderFillRect(renderer_, &rect);
}

void Dialog::render_box(int x, int y, int width, int height,
                        SDL_Color bg_color, SDL_Color border_color) {
    SDL_Rect box = {x, y, width, height};
    SDL_SetRenderDrawColor(renderer_, bg_color.r, bg_color.g, bg_color.b, bg_color.a);
    SDL_RenderFillRect(renderer_, &box);
    SDL_SetRenderDrawColor(renderer_, border_color.r, border_color.g, border_color.b, border_color.a);
    SDL_RenderDrawRect(renderer_, &box);
}

// MessageDialog
MessageDialog::MessageDialog(SDL_Renderer* r, TextRenderer* t) : Dialog(r, t) {}
MessageDialog::~MessageDialog() = default;
void MessageDialog::set_message(const std::string& title, const std::string& message) {
    title_ = title; message_ = message;
}
bool MessageDialog::process_input(SDL_Keycode key) {
    if (key == SDLK_RETURN || key == SDLK_ESCAPE) { hide(); return true; }
    return false;
}
void MessageDialog::render() {
    if (!visible_) return;
    render_overlay();
    int w, h;
    SDL_GetRendererOutputSize(renderer_, &w, &h);
    int bw = 400, bh = 200;
    render_box((w - bw) / 2, (h - bh) / 2, bw, bh, {40, 40, 40, 240}, {200, 200, 200, 255});
    SDL_Color white = {255, 255, 255, 255};
    text_renderer_->render_text(title_, (w - bw) / 2 + 20, (h - bh) / 2 + 20, white);
    text_renderer_->render_text(message_, (w - bw) / 2 + 20, (h - bh) / 2 + 60, white, TextRenderer::FontSize::Small);
}

// ConfirmDialog
ConfirmDialog::ConfirmDialog(SDL_Renderer* r, TextRenderer* t) : Dialog(r, t) {}
ConfirmDialog::~ConfirmDialog() = default;
void ConfirmDialog::set_message(const std::string& title, const std::string& message) {
    title_ = title; message_ = message;
}
bool ConfirmDialog::process_input(SDL_Keycode key) {
    if (key == SDLK_LEFT || key == SDLK_RIGHT) { selected_option_ = 1 - selected_option_; return true; }
    if (key == SDLK_RETURN) { result_ = (selected_option_ == 0); hide(); return true; }
    if (key == SDLK_ESCAPE) { result_ = false; hide(); return true; }
    return false;
}
void ConfirmDialog::render() {
    if (!visible_) return;
    render_overlay();
    // TODO: Render confirm dialog with Yes/No buttons
}

// ProgressDialog
ProgressDialog::ProgressDialog(SDL_Renderer* r, TextRenderer* t) : Dialog(r, t) {}
ProgressDialog::~ProgressDialog() = default;
void ProgressDialog::set_message(const std::string& message) { message_ = message; }
bool ProgressDialog::process_input(SDL_Keycode) { return false; }
void ProgressDialog::render() {
    if (!visible_) return;
    render_overlay();
    SDL_Color white = {255, 255, 255, 255};
    int w, h;
    SDL_GetRendererOutputSize(renderer_, &w, &h);
    text_renderer_->render_text(message_, w / 2 - 100, h / 2, white);
}
