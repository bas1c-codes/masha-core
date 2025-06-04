#pragma once
#include <string>
#include <unordered_set>

class Scan {
private:
    std::unordered_set<std::string> hashSet;
    bool hashesLoaded = false;
    void loadHashesIfNeeded();

public:
    void scan(const std::string& path);
};
