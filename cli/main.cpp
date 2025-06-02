#include <iostream>
#include "../core/hash.h"
#include "CLI/CLI.hpp"
int main(int argc, char **argv){
    Hash hash;
    CLI::App app;
    std::string path;
    app.add_option("-s",path,"Scan the file");
    CLI11_PARSE(app, argc, argv);
    std::string res=hash.hash(path);
    std::cout << res;
    
    return 0;     
}