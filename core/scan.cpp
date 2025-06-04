#include "scan.h"
#include "load.h"
#include "hash.h"
#include <iostream>

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

    if (sha256_hash.empty()) {
        std::cerr << "Error: Empty hash returned for file: " << path << "\n";
        return;
    }

    if (hashSet.count(sha256_hash)) {
        std::cout << "Malware found: " << path << "\n";
    } else {
        std::cout << "File is clean: " << path << "\n";
    }
}
