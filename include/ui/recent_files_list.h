#ifndef RECENT_FILES_LIST_H
#define RECENT_FILES_LIST_H

#include <string>
#include <vector>
#include <deque>

class RecentFilesList {
public:
    explicit RecentFilesList(size_t max_size = 10);

    void add(const std::string& filepath);
    bool remove(const std::string& filepath);
    bool contains(const std::string& filepath) const;
    std::vector<std::string> get_all() const;
    size_t size() const;
    bool empty() const;
    void clear();
    size_t max_size() const;

private:
    std::deque<std::string> files_;
    size_t max_size_;
};

#endif // RECENT_FILES_LIST_H
