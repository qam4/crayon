#ifndef FILE_BROWSER_H
#define FILE_BROWSER_H

#include <string>
#include <vector>
#include <SDL.h>

class TextRenderer;
class ConfigManager;

struct FileEntry {
    std::string name;
    std::string full_path;
    size_t size;
    bool is_directory;
};

class FileBrowser {
public:
    enum class FileType { Cartridge, ROM, Generic };

    FileBrowser(SDL_Renderer* renderer, TextRenderer* text_renderer, ConfigManager* config);
    ~FileBrowser();

    void open(const std::string& extensions, FileType file_type = FileType::Generic);
    void close();
    bool is_open() const { return is_open_; }
    bool process_input(SDL_Keycode key);
    void render();
    std::string get_selected_file() const { return selected_file_; }
    bool was_file_selected() const { return file_selected_; }

private:
    void scan_directory();
    bool matches_filter(const std::string& filename) const;
    void navigate_up();
    void navigate_into(const std::string& dir_name);
    void select_current_file();

    SDL_Renderer* renderer_;
    TextRenderer* text_renderer_;
    ConfigManager* config_;
    std::string current_directory_;
    std::string extensions_filter_;
    std::vector<FileEntry> entries_;
    int selected_index_ = 0;
    int scroll_offset_ = 0;
    bool is_open_ = false;
    bool file_selected_ = false;
    std::string selected_file_;
    FileType current_file_type_ = FileType::Generic;
};

#endif // FILE_BROWSER_H
