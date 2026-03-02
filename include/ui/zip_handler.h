#ifndef ZIP_HANDLER_H
#define ZIP_HANDLER_H

#include <string>
#include <vector>
#include <cstdint>

class ZIPHandler {
public:
    ZIPHandler();
    ~ZIPHandler();

    // ZIP file detection and opening
    static bool is_zip_file(const std::string& filepath);
    bool open(const std::string& zip_path);
    void close();
    bool is_open() const { return is_open_; }
    
    // Content listing
    std::vector<std::string> list_contents() const;
    
    // Extraction methods
    std::vector<uint8_t> extract_file(const std::string& filename);
    std::vector<uint8_t> extract_by_extension(const std::string& extension);
    
    // Temp file management
    void cleanup_temp_files();
    std::string get_temp_directory() const;

private:
    bool is_rom_file(const std::string& filename) const;
    bool has_extension(const std::string& filename, const std::string& extension) const;
    std::string create_temp_directory();

    std::string zip_path_;
    std::string temp_dir_;
    std::vector<std::string> temp_files_;
    bool is_open_ = false;
};

#endif // ZIP_HANDLER_H
