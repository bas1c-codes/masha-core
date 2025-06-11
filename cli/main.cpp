#include <iostream>
#include "../core/scan.h"
#include "CLI/CLI.hpp"
#include "../core/yara.h"
int main(int argc, char **argv){
    Scan scan;
    Yara yara;
    CLI::App app;
    std::string path;

    app.add_option("-s",path,"Scan the file");
    //app.add_option("-y",path,"Scan the file");
    CLI11_PARSE(app, argc, argv);
    scan.scan(path);
    //yara.yaraCheck(path);
    return 0;     
}