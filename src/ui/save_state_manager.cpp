#include "ui/save_state_manager.h"
#include "emulator_core.h"
#include "savestate.h"
#include <filesystem>
#include <ctime>

SaveStateManagerUI::SaveStateManagerUI(crayon::EmulatorCore* emulator, SDL_Renderer* renderer)
    : emulator_(emulator), renderer_(renderer) {
    ensure_saves_directory();
}

SaveStateManagerUI::~SaveStateManagerUI() = default;

crayon::Result<void> SaveStateManagerUI::save_state(int slot, const std::string& cartridge_name) {
    auto filename = get_state_filename(cartridge_name, slot);
    return emulator_->save_state(filename);
}

crayon::Result<void> SaveStateManagerUI::load_state(int slot, const std::string& cartridge_name) {
    auto filename = get_state_filename(cartridge_name, slot);
    return emulator_->load_state(filename);
}

crayon::Result<void> SaveStateManagerUI::delete_state(int slot, const std::string& cartridge_name) {
    auto filename = get_state_filename(cartridge_name, slot);
    if (std::filesystem::exists(filename)) {
        std::filesystem::remove(filename);
        auto thumb = get_thumbnail_filename(cartridge_name, slot);
        if (std::filesystem::exists(thumb)) std::filesystem::remove(thumb);
    }
    return crayon::Result<void>::ok();
}

std::vector<SaveStateInfo> SaveStateManagerUI::list_states(const std::string& cartridge_name) {
    std::vector<SaveStateInfo> states;
    for (int i = 0; i < 10; ++i) {
        SaveStateInfo info;
        info.slot = i;
        info.filename = get_state_filename(cartridge_name, i);
        info.thumbnail_path = get_thumbnail_filename(cartridge_name, i);
        info.exists = std::filesystem::exists(info.filename);
        if (info.exists) {
            auto ftime = std::filesystem::last_write_time(info.filename);
            auto sctp = std::chrono::time_point_cast<std::chrono::system_clock::duration>(
                ftime - std::filesystem::file_time_type::clock::now() + std::chrono::system_clock::now());
            info.timestamp = std::chrono::system_clock::to_time_t(sctp);
        } else {
            info.timestamp = 0;
        }
        states.push_back(info);
    }
    return states;
}

std::string SaveStateManagerUI::get_saves_directory() const { return saves_dir_; }

std::string SaveStateManagerUI::get_state_filename(const std::string& cartridge_name, int slot) {
    return saves_dir_ + "/" + cartridge_name + "_slot" + std::to_string(slot) + ".sav";
}

std::string SaveStateManagerUI::get_thumbnail_filename(const std::string& cartridge_name, int slot) {
    return saves_dir_ + "/" + cartridge_name + "_slot" + std::to_string(slot) + ".png";
}

crayon::Result<void> SaveStateManagerUI::capture_thumbnail(const std::string& /*filename*/) {
    // TODO: Capture framebuffer to PNG
    return crayon::Result<void>::ok();
}

bool SaveStateManagerUI::ensure_saves_directory() {
    saves_dir_ = "saves";
    if (!std::filesystem::exists(saves_dir_))
        std::filesystem::create_directories(saves_dir_);
    return std::filesystem::exists(saves_dir_);
}
