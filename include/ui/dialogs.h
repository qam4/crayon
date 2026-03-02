#ifndef DIALOGS_H
#define DIALOGS_H

#include <string>
#include <vector>
#include <SDL.h>

class TextRenderer;

class Dialog {
public:
    Dialog(SDL_Renderer* renderer, TextRenderer* text_renderer);
    virtual ~Dialog();
    virtual void show();
    virtual void hide();
    bool is_visible() const { return visible_; }
    virtual bool process_input(SDL_Keycode key) = 0;
    virtual void render() = 0;

protected:
    void render_overlay();
    void render_box(int x, int y, int width, int height,
                    SDL_Color bg_color, SDL_Color border_color);
    SDL_Renderer* renderer_;
    TextRenderer* text_renderer_;
    bool visible_ = false;
};

class MessageDialog : public Dialog {
public:
    MessageDialog(SDL_Renderer* renderer, TextRenderer* text_renderer);
    ~MessageDialog() override;
    void set_message(const std::string& title, const std::string& message);
    bool process_input(SDL_Keycode key) override;
    void render() override;
private:
    std::string title_;
    std::string message_;
};

class ConfirmDialog : public Dialog {
public:
    enum class Result { None, Yes, No };
    
    ConfirmDialog(SDL_Renderer* renderer, TextRenderer* text_renderer);
    ~ConfirmDialog() override;
    void set_message(const std::string& title, const std::string& message);
    Result get_result() const { return result_; }
    bool process_input(SDL_Keycode key) override;
    void render() override;
    
private:
    std::string title_;
    std::string message_;
    int selected_option_ = 0; // 0 = Yes, 1 = No
    Result result_ = Result::None;
};

class ProgressDialog : public Dialog {
public:
    ProgressDialog(SDL_Renderer* renderer, TextRenderer* text_renderer);
    ~ProgressDialog() override;
    void set_message(const std::string& message);
    void set_progress(float progress); // 0.0 to 1.0
    bool process_input(SDL_Keycode key) override;
    void render() override;
    
private:
    std::string message_;
    float progress_ = 0.0f;
};

#endif // DIALOGS_H
