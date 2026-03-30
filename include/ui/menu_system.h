#ifndef MENU_SYSTEM_H
#define MENU_SYSTEM_H

#include <string>
#include <vector>
#include <stack>
#include <SDL.h>
#include "frontend.h"

class TextRenderer;
class ConfigManager;
class SaveStateManagerUI;

struct MenuItem {
    std::string label;
    crayon::MenuAction action;
    std::vector<MenuItem> submenu;
    bool enabled;
    bool has_submenu;
    int slot_number;
    std::string value;

    MenuItem(const std::string& lbl, crayon::MenuAction act = crayon::MenuAction::None, bool en = true)
        : label(lbl), action(act), enabled(en), has_submenu(false), slot_number(-1) {}
};

class MenuSystem {
public:
    MenuSystem(SDL_Renderer* renderer, TextRenderer* text_renderer);
    ~MenuSystem();

    void show();
    void hide();
    bool is_visible() const { return visible_; }
    void open() { show(); }
    void close() { hide(); }
    bool is_open() const { return visible_; }
    crayon::MenuAction process_input(SDL_Keycode key);
    int get_selected_slot() const;
    void render();
    void build_main_menu();
    void update_menu_values(ConfigManager* config_manager);
    void update_save_state_slots(SaveStateManagerUI* save_state_manager, const std::string& game_name);

private:
    void navigate_up();
    void navigate_down();
    void select_current();
    void go_back();
    void render_menu_list(const std::vector<MenuItem>& items, int selected_index);

    SDL_Renderer* renderer_;
    TextRenderer* text_renderer_;
    std::vector<MenuItem> main_menu_;
    std::vector<MenuItem>* current_menu_ = nullptr;
    std::stack<std::vector<MenuItem>*> menu_stack_;
    int selected_index_ = 0;
    int scroll_offset_ = 0;
    int last_selected_slot_ = -1;
    bool visible_ = false;
};

#endif // MENU_SYSTEM_H
