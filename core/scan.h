#pragma once
#include <string>
#include <unordered_set>

class Scan {
private:
    std::unordered_set<std::string> hashSet;
    bool hashesLoaded = false;
    bool objectLoaded = false;
    void loadHashesIfNeeded();
    void scanFile(const std::string& filePath);

public:
    void scan(const std::string& path);
};
