#include "ui/zip_handler.h"
#include <filesystem>
#include <fstream>
#include <algorithm>

namespace fs = std::filesystem;

ZIPHandler::ZIPHandler() = default;

ZIPHandler::~ZIPHandler() { 
    cleanup_temp_files(); 
}

bool ZIPHandler::is_zip_file(const std::string& filepath) {
    // Check file extension
    std::string ext = fs::path(filepath).extension().string();
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
    if (ext != ".zip") return false;
    
    // Check ZIP magic number (PK\x03\x04)
    std::ifstream file(filepath, std::ios::binary);
    if (!file.is_open()) return false;
    
    char magic[4];
    file.read(magic, 4);
    if (file.gcount() != 4) return false;
    
    return (magic[0] == 'P' && magic[1] == 'K' && 
            magic[2] == 0x03 && magic[3] == 0x04);
}

bool ZIPHandler::open(const std::string& zip_path) {
    if (!fs::exists(zip_path)) return false;
    if (!is_zip_file(zip_path)) return false;
    
    zip_path_ = zip_path;
    is_open_ = true;
    temp_dir_ = create_temp_directory();
    
    return true;
}

void ZIPHandler::close() {
    is_open_ = false;
    cleanup_temp_files();
}

std::vector<std::string> ZIPHandler::list_contents() const {
    if (!is_open_) return {};
    
    // NOTE: This is a stub implementation
    // A full implementation would use a ZIP library like miniz or libzip
    // to enumerate the contents of the ZIP file
    
    // For now, return empty list
    // In a real implementation, this would parse the ZIP central directory
    return {};
}

std::vector<uint8_t> ZIPHandler::extract_file(const std::string& filename) {
    if (!is_open_) return {};
    
    // NOTE: This is a stub implementation
    // A full implementation would use a ZIP library to extract the file
    
    // For now, return empty vector
    // In a real implementation, this would:
    // 1. Locate the file in the ZIP central directory
    // 2. Read the compressed data
    // 3. Decompress it (if compressed)
    // 4. Return the uncompressed data
    
    (void)filename;
    return {};
}

std::vector<uint8_t> ZIPHandler::extract_by_extension(const std::string& extension) {
    if (!is_open_) return {};
    
    // List all files in the ZIP
    auto files = list_contents();
    
    // Find first file matching the extension
    for (const auto& file : files) {
        if (has_extension(file, extension)) {
            return extract_file(file);
        }
    }
    
    return {};
}

void ZIPHandler::cleanup_temp_files() {
    // Remove all temporary files
    for (const auto& file : temp_files_) {
        if (fs::exists(file)) {
            fs::remove(file);
        }
    }
    temp_files_.clear();
    
    // Remove temp directory if it exists and is empty
    if (!temp_dir_.empty() && fs::exists(temp_dir_)) {
        try {
            fs::remove(temp_dir_);
        } catch (...) {
            // Directory might not be empty, ignore
        }
    }
}

std::string ZIPHandler::get_temp_directory() const {
    return temp_dir_;
}

bool ZIPHandler::is_rom_file(const std::string& filename) const {
    return has_extension(filename, ".rom") || 
           has_extension(filename, ".bin") ||
           has_extension(filename, ".k7");
}

bool ZIPHandler::has_extension(const std::string& filename, const std::string& extension) const {
    std::string file_ext = fs::path(filename).extension().string();
    std::string target_ext = extension;
    
    // Convert both to lowercase for case-insensitive comparison
    std::transform(file_ext.begin(), file_ext.end(), file_ext.begin(), ::tolower);
    std::transform(target_ext.begin(), target_ext.end(), target_ext.begin(), ::tolower);
    
    // Ensure target extension has leading dot
    if (!target_ext.empty() && target_ext[0] != '.') {
        target_ext = "." + target_ext;
    }
    
    return file_ext == target_ext;
}

std::string ZIPHandler::create_temp_directory() {
    // Create a temp directory in the system temp location
    fs::path temp_path = fs::temp_directory_path() / "crayon_zip_temp";
    
    // Create directory if it doesn't exist
    if (!fs::exists(temp_path)) {
        fs::create_directories(temp_path);
    }
    
    return temp_path.string();
}
