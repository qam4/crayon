#ifndef UI_SAVE_STATE_MANAGER_H
#define UI_SAVE_STATE_MANAGER_H

#include "types.h"
#include <string>
#include <vector>
#include <ctime>
#include <SDL.h>

namespace crayon {
    class EmulatorCore;
}

class TextRenderer;

struct SaveStateInfo {
    int slot;
    std::string filename;
    std::string thumbnail_path;
    std::time_t timestamp;
    bool exists;
    SDL_Texture* thumbnail_texture = nullptr; // Cached thumbnail texture
};

class SaveStateManagerUI {
public:
    SaveStateManagerUI(crayon::EmulatorCore* emulator, SDL_Renderer* renderer, TextRenderer* text_renderer);
    ~SaveStateManagerUI();

    // Core save state operations
    crayon::Result<void> save_state(int slot, const std::string& cartridge_name);
    crayon::Result<void> load_state(int slot, const std::string& cartridge_name);
    crayon::Result<void> delete_state(int slot, const std::string& cartridge_name);
    std::vector<SaveStateInfo> list_states(const std::string& cartridge_name);
    std::string get_saves_directory() const;
    
    // UI rendering
    void render_ui(const std::string& cartridge_name);
    bool process_input(SDL_Keycode key);
    void show_ui(bool show) { show_ui_ = show; }
    bool is_ui_visible() const { return show_ui_; }
    void set_mode(bool is_save_mode) { is_save_mode_ = is_save_mode; }

private:
    std::string get_state_filename(const std::string& cartridge_name, int slot);
    std::string get_thumbnail_filename(const std::string& cartridge_name, int slot);
    crayon::Result<void> capture_thumbnail(const std::string& filename);
    bool ensure_saves_directory();
    void load_thumbnail_textures(const std::vector<SaveStateInfo>& states);
    void free_thumbnail_textures();
    std::string format_timestamp(std::time_t timestamp) const;

    crayon::EmulatorCore* emulator_;
    SDL_Renderer* renderer_;
    TextRenderer* text_renderer_;
    std::string saves_dir_;
    
    // UI state
    bool show_ui_ = false;
    bool is_save_mode_ = true; // true = save, false = load
    int selected_slot_ = 0;
    std::vector<SaveStateInfo> cached_states_;
};

#endif // UI_SAVE_STATE_MANAGER_H
