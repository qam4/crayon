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

struct SaveStateInfo {
    int slot;
    std::string filename;
    std::string thumbnail_path;
    std::time_t timestamp;
    bool exists;
};

class SaveStateManagerUI {
public:
    SaveStateManagerUI(crayon::EmulatorCore* emulator, SDL_Renderer* renderer);
    ~SaveStateManagerUI();

    crayon::Result<void> save_state(int slot, const std::string& cartridge_name);
    crayon::Result<void> load_state(int slot, const std::string& cartridge_name);
    crayon::Result<void> delete_state(int slot, const std::string& cartridge_name);
    std::vector<SaveStateInfo> list_states(const std::string& cartridge_name);
    std::string get_saves_directory() const;

private:
    std::string get_state_filename(const std::string& cartridge_name, int slot);
    std::string get_thumbnail_filename(const std::string& cartridge_name, int slot);
    crayon::Result<void> capture_thumbnail(const std::string& filename);
    bool ensure_saves_directory();

    crayon::EmulatorCore* emulator_;
    SDL_Renderer* renderer_;
    std::string saves_dir_;
};

#endif // UI_SAVE_STATE_MANAGER_H
