#include "ui/dialogs.h"
#include "ui/text_renderer.h"
#include <algorithm>

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
    title_ = title; 
    message_ = message;
}

bool MessageDialog::process_input(SDL_Keycode key) {
    if (key == SDLK_RETURN || key == SDLK_ESCAPE || key == SDLK_SPACE) { 
        hide(); 
        return true; 
    }
    return false;
}

void MessageDialog::render() {
    if (!visible_) return;
    
    render_overlay();
    
    int window_width, window_height;
    SDL_GetRendererOutputSize(renderer_, &window_width, &window_height);
    
    int box_width = 500;
    int box_height = 200;
    int box_x = (window_width - box_width) / 2;
    int box_y = (window_height - box_height) / 2;
    
    // Render dialog box
    render_box(box_x, box_y, box_width, box_height, 
               {40, 40, 40, 240}, {200, 200, 200, 255});
    
    // Render title
    SDL_Color title_color = {255, 255, 255, 255};
    text_renderer_->render_text(renderer_, title_.c_str(), 
                                box_x + 20, box_y + 20, 
                                title_color, TextRenderer::TextAlign::Left);
    
    // Render message
    SDL_Color message_color = {220, 220, 220, 255};
    text_renderer_->render_text(renderer_, message_.c_str(), 
                                box_x + 20, box_y + 60, 
                                message_color, TextRenderer::TextAlign::Left);
    
    // Render OK button
    SDL_Color button_color = {180, 180, 180, 255};
    text_renderer_->render_text(renderer_, "[Press ENTER to continue]", 
                                box_x + box_width / 2, box_y + box_height - 40, 
                                button_color, TextRenderer::TextAlign::Center);
}

// ConfirmDialog
ConfirmDialog::ConfirmDialog(SDL_Renderer* r, TextRenderer* t) : Dialog(r, t) {}
ConfirmDialog::~ConfirmDialog() = default;

void ConfirmDialog::set_message(const std::string& title, const std::string& message) {
    title_ = title; 
    message_ = message;
    selected_option_ = 0;
    result_ = Result::None;
}

bool ConfirmDialog::process_input(SDL_Keycode key) {
    if (key == SDLK_LEFT || key == SDLK_RIGHT || key == SDLK_TAB) { 
        selected_option_ = 1 - selected_option_; 
        return true; 
    }
    if (key == SDLK_RETURN || key == SDLK_SPACE) { 
        result_ = (selected_option_ == 0) ? Result::Yes : Result::No;
        hide(); 
        return true; 
    }
    if (key == SDLK_ESCAPE) { 
        result_ = Result::No;
        hide(); 
        return true; 
    }
    return false;
}

void ConfirmDialog::render() {
    if (!visible_) return;
    
    render_overlay();
    
    int window_width, window_height;
    SDL_GetRendererOutputSize(renderer_, &window_width, &window_height);
    
    int box_width = 500;
    int box_height = 220;
    int box_x = (window_width - box_width) / 2;
    int box_y = (window_height - box_height) / 2;
    
    // Render dialog box
    render_box(box_x, box_y, box_width, box_height, 
               {40, 40, 40, 240}, {200, 200, 200, 255});
    
    // Render title
    SDL_Color title_color = {255, 255, 255, 255};
    text_renderer_->render_text(renderer_, title_.c_str(), 
                                box_x + 20, box_y + 20, 
                                title_color, TextRenderer::TextAlign::Left);
    
    // Render message
    SDL_Color message_color = {220, 220, 220, 255};
    text_renderer_->render_text(renderer_, message_.c_str(), 
                                box_x + 20, box_y + 60, 
                                message_color, TextRenderer::TextAlign::Left);
    
    // Render Yes button
    int button_y = box_y + box_height - 50;
    int yes_x = box_x + box_width / 2 - 80;
    int no_x = box_x + box_width / 2 + 20;
    
    if (selected_option_ == 0) {
        // Highlight Yes button
        SDL_SetRenderDrawColor(renderer_, 60, 60, 120, 255);
        SDL_Rect highlight = {yes_x - 5, button_y - 2, 60, 24};
        SDL_RenderFillRect(renderer_, &highlight);
    }
    SDL_Color yes_color = (selected_option_ == 0) ? SDL_Color{255, 255, 255, 255} : SDL_Color{180, 180, 180, 255};
    text_renderer_->render_text(renderer_, "Yes", yes_x, button_y, yes_color, TextRenderer::TextAlign::Left);
    
    if (selected_option_ == 1) {
        // Highlight No button
        SDL_SetRenderDrawColor(renderer_, 60, 60, 120, 255);
        SDL_Rect highlight = {no_x - 5, button_y - 2, 50, 24};
        SDL_RenderFillRect(renderer_, &highlight);
    }
    SDL_Color no_color = (selected_option_ == 1) ? SDL_Color{255, 255, 255, 255} : SDL_Color{180, 180, 180, 255};
    text_renderer_->render_text(renderer_, "No", no_x, button_y, no_color, TextRenderer::TextAlign::Left);
    
    // Render instructions
    SDL_Color instruction_color = {150, 150, 150, 255};
    text_renderer_->render_text(renderer_, "[LEFT/RIGHT: Select | ENTER: Confirm | ESC: Cancel]", 
                                box_x + box_width / 2, box_y + box_height - 20, 
                                instruction_color, TextRenderer::TextAlign::Center);
}

// ProgressDialog
ProgressDialog::ProgressDialog(SDL_Renderer* r, TextRenderer* t) : Dialog(r, t) {}
ProgressDialog::~ProgressDialog() = default;

void ProgressDialog::set_message(const std::string& message) { 
    message_ = message; 
}

void ProgressDialog::set_progress(float progress) {
    progress_ = std::clamp(progress, 0.0f, 1.0f);
}

bool ProgressDialog::process_input(SDL_Keycode) { 
    // Progress dialog doesn't accept input
    return false; 
}

void ProgressDialog::render() {
    if (!visible_) return;
    
    render_overlay();
    
    int window_width, window_height;
    SDL_GetRendererOutputSize(renderer_, &window_width, &window_height);
    
    int box_width = 500;
    int box_height = 150;
    int box_x = (window_width - box_width) / 2;
    int box_y = (window_height - box_height) / 2;
    
    // Render dialog box
    render_box(box_x, box_y, box_width, box_height, 
               {40, 40, 40, 240}, {200, 200, 200, 255});
    
    // Render message
    SDL_Color message_color = {255, 255, 255, 255};
    text_renderer_->render_text(renderer_, message_.c_str(), 
                                box_x + box_width / 2, box_y + 30, 
                                message_color, TextRenderer::TextAlign::Center);
    
    // Render progress bar background
    int bar_width = 400;
    int bar_height = 30;
    int bar_x = box_x + (box_width - bar_width) / 2;
    int bar_y = box_y + 70;
    
    SDL_SetRenderDrawColor(renderer_, 60, 60, 60, 255);
    SDL_Rect bar_bg = {bar_x, bar_y, bar_width, bar_height};
    SDL_RenderFillRect(renderer_, &bar_bg);
    
    // Render progress bar fill
    int fill_width = static_cast<int>(bar_width * progress_);
    if (fill_width > 0) {
        SDL_SetRenderDrawColor(renderer_, 80, 120, 200, 255);
        SDL_Rect bar_fill = {bar_x, bar_y, fill_width, bar_height};
        SDL_RenderFillRect(renderer_, &bar_fill);
    }
    
    // Render progress bar border
    SDL_SetRenderDrawColor(renderer_, 150, 150, 150, 255);
    SDL_RenderDrawRect(renderer_, &bar_bg);
    
    // Render percentage text
    char percent_text[16];
    snprintf(percent_text, sizeof(percent_text), "%.0f%%", progress_ * 100.0f);
    SDL_Color percent_color = {255, 255, 255, 255};
    text_renderer_->render_text(renderer_, percent_text, 
                                box_x + box_width / 2, bar_y + 7, 
                                percent_color, TextRenderer::TextAlign::Center);
}
