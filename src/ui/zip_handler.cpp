#include "ui/zip_handler.h"

ZIPHandler::ZIPHandler() = default;
ZIPHandler::~ZIPHandler() { cleanup_temp_files(); }

bool ZIPHandler::open(const std::string& zip_path) {
#ifdef HAVE_MINIZ
    // TODO: Implement with miniz
    zip_path_ = zip_path;
    is_open_ = true;
    return true;
#else
    (void)zip_path;
    return false;
#endif
}

void ZIPHandler::close() {
#ifdef HAVE_MINIZ
    is_open_ = false;
#endif
}

std::vector<std::string> ZIPHandler::get_rom_files() const {
    return {};
}

std::string ZIPHandler::extract_file(const std::string& /*filename*/) {
    return "";
}

std::vector<std::string> ZIPHandler::extract_all_roms() {
    return {};
}

void ZIPHandler::cleanup_temp_files() {
#ifdef HAVE_MINIZ
    temp_files_.clear();
#endif
}

std::string ZIPHandler::get_temp_directory() const {
#ifdef HAVE_MINIZ
    return temp_dir_;
#else
    return "";
#endif
}
