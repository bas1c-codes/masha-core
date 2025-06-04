#ifndef LOAD_H
#define LOAD_H
#include <unordered_set>
#include <string>
#include <iostream>
class LoadHash{
    public:
        std::unordered_set<std::string> load();
};
#endif