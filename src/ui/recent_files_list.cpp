#include "ui/recent_files_list.h"
#include <algorithm>

RecentFilesList::RecentFilesList(size_t max_size) : max_size_(max_size) {}

void RecentFilesList::add(const std::string& filepath) {
    auto it = std::find(files_.begin(), files_.end(), filepath);
    if (it != files_.end()) files_.erase(it);
    files_.push_front(filepath);
    while (files_.size() > max_size_) files_.pop_back();
}

bool RecentFilesList::remove(const std::string& filepath) {
    auto it = std::find(files_.begin(), files_.end(), filepath);
    if (it == files_.end()) return false;
    files_.erase(it);
    return true;
}

bool RecentFilesList::contains(const std::string& filepath) const {
    return std::find(files_.begin(), files_.end(), filepath) != files_.end();
}

std::vector<std::string> RecentFilesList::get_all() const {
    return {files_.begin(), files_.end()};
}

size_t RecentFilesList::size() const { return files_.size(); }
bool RecentFilesList::empty() const { return files_.empty(); }
void RecentFilesList::clear() { files_.clear(); }
size_t RecentFilesList::max_size() const { return max_size_; }
