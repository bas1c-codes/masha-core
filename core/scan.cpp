#include "scan.h"
#include "load.h"
#include "hash.h"
#include "quarantine.h"
#include <iostream>
#include <filesystem>
#include "yara.h"
void Scan::loadHashesIfNeeded() {
    if (!hashesLoaded) {
        LoadHash loader;
        hashSet = loader.load();

        if (hashSet.empty()) {
            std::cerr << "Warning: Hash database is empty or failed to load.\n";
        } else {
            hashesLoaded = true;
            std::cout << "Loaded " << hashSet.size() << " hashes into memory.\n";
        }
    }
}

// Helper function to scan a single file
void Scan::scan(const std::string& filePath) {
    loadHashesIfNeeded();
    if (hashSet.empty()) {
        std::cerr << "Error: No hashes loaded, cannot scan.\n";
        return;
    }

    Hash sha256;
    std::string sha256_hash = sha256.hash(filePath);
    Quarantine quarantine;
    if (sha256_hash == "File not found" ||
        sha256_hash == "I/O error while reading file" ||
        sha256_hash == "Non-EOF read error") {
        std::cerr << "Error hashing file '" << filePath << "': " << sha256_hash << "\n";
        return;
    }

    if (hashSet.count(sha256_hash)) {
        std::cout << "Malware found: " << filePath << "\n";
        quarantine.quarantineMalware(filePath);

    }
    else {
        Yara yara;
        std::cout << "Checking with yara";
        yara.yaraCheck(filePath);
    }
}

// Main scanning function
/*void Scan::scan(const std::string& path) {
    loadHashesIfNeeded();

    namespace fs = std::filesystem;
    std::error_code ec;
    fs::path fpath(path);

    if (fs::is_directory(fpath, ec)) {
        if (ec) {
            std::cerr << "Error accessing directory: " << ec.message() << "\n";
            return;
        }

        for (fs::recursive_directory_iterator it(fpath, fs::directory_options::skip_permission_denied, ec), end;
             it != end;
             it.increment(ec)) {

            if (ec) {
                std::cerr << "Iterator error: " << ec.message() << "\n";
                continue;
            }

            const fs::path& currentPath = it->path();

            if (fs::is_symlink(currentPath, ec) && !fs::exists(currentPath, ec)) {
                std::cerr << "Skipping broken symlink: " << currentPath << "\n";
                continue;
            }

            if (fs::is_regular_file(currentPath, ec)) {
                scanFile(currentPath.string());
            }
        }
    } else if (fs::is_regular_file(fpath, ec)) {
        scanFile(path);
    } else {
        std::cerr << "Path is neither a file nor a directory or is inaccessible: " << path << "\n";
    }
}*/
