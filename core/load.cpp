#include <iostream>
#include "load.h"
#include <unordered_set>
#include <fstream>
#include <string>
std::unordered_set<std::string> LoadHash::load(){
    std::ifstream file("mashadb.mh",std::ios::binary);
    std::unordered_set<std::string> hash;
    if(!file){
        std::cerr << "mashadb not opening";
        return hash;
    }
    std::string line;
    while (std::getline(file, line)) {
        if (!line.empty() && line.back() == '\r') // Handle CR from Windows line endings
            line.pop_back();

        if (line.length() == 64) { // SHA-256 in hex
            hash.insert(line);
        } else {
            std::cerr << "Invalid hash: " << line << '\n';
        }
    }
    return hash;
};