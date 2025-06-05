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
    /*for checking if give path is a directory or not*/
    namespace fs = std::filesystem;
    try{
    fs::path fpath = path;
    if(fs::is_directory(fpath)){
            for (const auto& entry : fs::recursive_directory_iterator(fpath, fs::directory_options::skip_permission_denied)) {
                try {
                            if (fs::is_symlink(entry.path()) && !fs::exists(entry.path())) {
            std::cerr << "Skipping broken symlink: " << entry.path() << "\n";
            continue;
        }
                if (fs::is_regular_file(entry.path())) {
                    scan(entry.path().string());
                 }
                 } catch (const std::exception& e) {
                    std::cerr << "Error accessing " << entry.path() << ": " << e.what() << "\n";
                }
            }
    }
    else{  /*do it normal way if it is a file*/

    if (hashSet.empty()) {  /*checking if set is empty (db could not be found probably)*/
        std::cerr << "Error: No hashes loaded, cannot scan.\n";
        return;
    }
    Hash sha256;
    std::string sha256_hash = sha256.hash(path);

    if (sha256_hash == "File not found" ||  /*checking for has errors*/
        sha256_hash == "I/O error while reading file" ||
        sha256_hash == "Non-EOF read error") {
        std::cerr << "Error hashing file '" << path << "': " << sha256_hash << "\n";
        return;
    }
    if (hashSet.count(sha256_hash)) {   /*checking if hash is present in memory*/
        std::cout << "Malware found: " << path << "\n";
    } /*else {
        std::cout << "File is clean" << "\n";
    }*/
    }
    }
    catch(const std::exception& e){
        std::cerr << e.what();
    }
}
