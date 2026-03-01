#ifndef ZIP_HANDLER_H
#define ZIP_HANDLER_H

#include <string>
#include <vector>

class ZIPHandler {
public:
    ZIPHandler();
    ~ZIPHandler();

    bool open(const std::string& zip_path);
    void close();
    std::vector<std::string> get_rom_files() const;
    std::string extract_file(const std::string& filename);
    std::vector<std::string> extract_all_roms();
    void cleanup_temp_files();
    std::string get_temp_directory() const;

private:
#ifdef HAVE_MINIZ
    void* zip_archive_ = nullptr;
    std::string zip_path_;
    std::string temp_dir_;
    std::vector<std::string> temp_files_;
    bool is_open_ = false;

    bool is_rom_file(const std::string& filename) const;
    std::string create_temp_directory();
    bool extract_to_path(const std::string& filename, const std::string& output_path);
#endif
};

#endif // ZIP_HANDLER_H
