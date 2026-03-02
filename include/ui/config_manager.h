#ifndef CONFIG_MANAGER_H
#define CONFIG_MANAGER_H

#include <string>
#include <map>
#include <vector>
#include <cstdint>
#include "ui/recent_files_list.h"

class ConfigManager {
public:
    ConfigManager();
    ~ConfigManager();

    bool load();
    bool save();
    std::string get_config_path() const;

    std::string get_last_cartridge_directory() const;
    void set_last_cartridge_directory(const std::string& path);
    std::string get_last_rom_directory() const;
    void set_last_rom_directory(const std::string& path);
    std::string get_last_k7_directory() const;
    void set_last_k7_directory(const std::string& path);

    bool get_auto_load_last_files() const;
    void set_auto_load_last_files(bool enabled);

    std::string get_scaling_filter() const;
    void set_scaling_filter(const std::string& filter);
    std::string get_aspect_ratio() const;
    void set_aspect_ratio(const std::string& ratio);
    bool get_fullscreen() const;
    void set_fullscreen(bool enabled);
    int get_window_scale() const;
    void set_window_scale(int scale);

    int get_volume() const;
    void set_volume(int volume);
    bool get_audio_muted() const;
    void set_audio_muted(bool muted);
    int get_audio_buffer_size() const;
    void set_audio_buffer_size(int size);

    std::string get_fps_position() const;
    void set_fps_position(const std::string& position);
    bool get_fps_display_enabled() const;
    void set_fps_display_enabled(bool enabled);

    std::vector<std::string> get_recent_cartridges() const;
    void add_recent_cartridge(const std::string& path);

private:
    bool parse_ini_file(const std::string& filepath);
    bool write_ini_file(const std::string& filepath);

    std::string get_value(const std::string& section, const std::string& key, const std::string& default_value) const;
    int get_value_int(const std::string& section, const std::string& key, int default_value) const;
    bool get_value_bool(const std::string& section, const std::string& key, bool default_value) const;
    void set_value(const std::string& section, const std::string& key, const std::string& value);

    std::map<std::string, std::map<std::string, std::string>> config_;
    std::string config_path_;
    RecentFilesList recent_cartridges_;
};

#endif // CONFIG_MANAGER_H
