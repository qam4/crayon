#include "ui/config_manager.h"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <filesystem>

ConfigManager::ConfigManager() : recent_cartridges_(10) {
    // Ensure userdata directory exists
    std::filesystem::create_directories("userdata");
}
ConfigManager::~ConfigManager() = default;

bool ConfigManager::load() { return parse_ini_file(get_config_path()); }
bool ConfigManager::save() { return write_ini_file(get_config_path()); }

std::string ConfigManager::get_config_path() const {
    return "userdata/config.ini";
}

std::string ConfigManager::get_last_cartridge_directory() const { return get_value("General", "last_cartridge_dir", ""); }
void ConfigManager::set_last_cartridge_directory(const std::string& path) { set_value("General", "last_cartridge_dir", path); }
std::string ConfigManager::get_last_rom_directory() const { return get_value("General", "last_rom_dir", ""); }
void ConfigManager::set_last_rom_directory(const std::string& path) { set_value("General", "last_rom_dir", path); }
bool ConfigManager::get_auto_load_last_files() const { return get_value_bool("General", "auto_load", false); }
void ConfigManager::set_auto_load_last_files(bool enabled) { set_value("General", "auto_load", enabled ? "true" : "false"); }

std::string ConfigManager::get_scaling_filter() const { return get_value("Video", "scaling_filter", "nearest"); }
void ConfigManager::set_scaling_filter(const std::string& filter) { set_value("Video", "scaling_filter", filter); }
std::string ConfigManager::get_aspect_ratio() const { return get_value("Video", "aspect_ratio", "4:3"); }
void ConfigManager::set_aspect_ratio(const std::string& ratio) { set_value("Video", "aspect_ratio", ratio); }
bool ConfigManager::get_fullscreen() const { return get_value_bool("Video", "fullscreen", false); }
void ConfigManager::set_fullscreen(bool enabled) { set_value("Video", "fullscreen", enabled ? "true" : "false"); }

int ConfigManager::get_volume() const { return get_value_int("Audio", "volume", 70); }
void ConfigManager::set_volume(int volume) { set_value("Audio", "volume", std::to_string(volume)); }
bool ConfigManager::get_audio_muted() const { return get_value_bool("Audio", "muted", false); }
void ConfigManager::set_audio_muted(bool muted) { set_value("Audio", "muted", muted ? "true" : "false"); }
int ConfigManager::get_audio_buffer_size() const { return get_value_int("Audio", "buffer_size", 512); }
void ConfigManager::set_audio_buffer_size(int size) { set_value("Audio", "buffer_size", std::to_string(size)); }

std::string ConfigManager::get_fps_position() const { return get_value("OSD", "fps_position", "top-right"); }
void ConfigManager::set_fps_position(const std::string& position) { set_value("OSD", "fps_position", position); }
bool ConfigManager::get_fps_display_enabled() const { return get_value_bool("OSD", "fps_enabled", true); }
void ConfigManager::set_fps_display_enabled(bool enabled) { set_value("OSD", "fps_enabled", enabled ? "true" : "false"); }

std::vector<std::string> ConfigManager::get_recent_cartridges() const { return recent_cartridges_.get_all(); }
void ConfigManager::add_recent_cartridge(const std::string& path) { recent_cartridges_.add(path); }

bool ConfigManager::parse_ini_file(const std::string& filepath) {
    std::ifstream file(filepath);
    if (!file) return false;
    std::string line, current_section;
    while (std::getline(file, line)) {
        if (line.empty() || line[0] == ';' || line[0] == '#') continue;
        if (line[0] == '[') {
            auto end = line.find(']');
            if (end != std::string::npos) current_section = line.substr(1, end - 1);
        } else {
            auto eq = line.find('=');
            if (eq != std::string::npos)
                config_[current_section][line.substr(0, eq)] = line.substr(eq + 1);
        }
    }
    return true;
}

bool ConfigManager::write_ini_file(const std::string& filepath) {
    std::ofstream file(filepath);
    if (!file) return false;
    for (const auto& section : config_) {
        file << "[" << section.first << "]\n";
        for (const auto& kv : section.second)
            file << kv.first << "=" << kv.second << "\n";
        file << "\n";
    }
    return true;
}

std::string ConfigManager::get_value(const std::string& section, const std::string& key, const std::string& def) const {
    auto s = config_.find(section);
    if (s == config_.end()) return def;
    auto k = s->second.find(key);
    return (k != s->second.end()) ? k->second : def;
}

int ConfigManager::get_value_int(const std::string& section, const std::string& key, int def) const {
    auto val = get_value(section, key, "");
    if (val.empty()) return def;
    try { return std::stoi(val); } catch (...) { return def; }
}

bool ConfigManager::get_value_bool(const std::string& section, const std::string& key, bool def) const {
    auto val = get_value(section, key, "");
    if (val.empty()) return def;
    return val == "true" || val == "1" || val == "yes";
}

void ConfigManager::set_value(const std::string& section, const std::string& key, const std::string& value) {
    config_[section][key] = value;
}
