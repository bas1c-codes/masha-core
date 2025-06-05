#include "scan.h"
#include "load.h"
#include "hash.h"
#include <iostream>
#include <filesystem>

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

void Scan::scan(const std::string& path) {
    loadHashesIfNeeded();

    namespace fs = std::filesystem;
    try {
        fs::path fpath = path;

        if (fs::is_directory(fpath)) {
            fs::recursive_directory_iterator dirIter(fpath, fs::directory_options::skip_permission_denied);
            fs::recursive_directory_iterator endIter;

            while (dirIter != endIter) {
                try {
                    const auto& entry = *dirIter;

                    // Skip broken symlinks
                    if (fs::is_symlink(entry.path()) && !fs::exists(entry.path())) {
                        std::cerr << "Skipping broken symlink: " << entry.path() << "\n";
                        ++dirIter;
                        continue;
                    }

                    if (fs::is_regular_file(entry.path())) {
                        scan(entry.path().string());
                    }

                    ++dirIter;  // May throw, so inside try-catch
                }
                catch (const std::exception& e) {
                    std::cerr << "Error accessing directory entry: " << e.what() << "\n";
                    // Try to increment iterator to move past problematic entry
                    try {
                        ++dirIter;
                    }
                    catch (...) {
                        std::cerr << "Failed to increment directory iterator after exception.\n";
                        break;
                    }
                }
            }
        }
        else {
            // If path is a file
            if (hashSet.empty()) {
                std::cerr << "Error: No hashes loaded, cannot scan.\n";
                return;
            }

            Hash sha256;
            std::string sha256_hash = sha256.hash(path);

            if (sha256_hash == "File not found" ||
                sha256_hash == "I/O error while reading file" ||
                sha256_hash == "Non-EOF read error") {
                std::cerr << "Error hashing file '" << path << "': " << sha256_hash << "\n";
                return;
            }

            if (hashSet.count(sha256_hash)) {
                std::cout << "Malware found: " << path << "\n";
            }
            // else file is clean, you can uncomment below if you want output
            // else {
            //    std::cout << "File is clean: " << path << "\n";
            // }
        }
    }
    catch (const std::exception& e) {
        std::cerr << "Error scanning path '" << path << "': " << e.what() << "\n";
    }
}
