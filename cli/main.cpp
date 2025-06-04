#include <iostream>
#include "../core/scan.h"
#include "CLI/CLI.hpp"
int main(int argc, char **argv){
    Scan scan;
    CLI::App app;
    std::string path;
    app.add_option("-s",path,"Scan the file");
    CLI11_PARSE(app, argc, argv);
    scan.scan(path);
    
    return 0;     
}