#include <iostream>
#include <fstream>
#include "yara.h"
#include "quarantine.h"
#include <yara/libyara.h>
#include <yara/scan.h>
#include <yara/rules.h>
#include <yara/compiler.h>

struct CallbackContext {
    std::string path;
    Quarantine* quarantine;
    bool matchFound = false;
};

int yaraCallback(YR_SCAN_CONTEXT* context, int message, void* messageData, void* userData) {
    if (message == CALLBACK_MSG_RULE_MATCHING) {
        auto* ctx = static_cast<CallbackContext*>(userData);
        YR_RULE* rule = static_cast<YR_RULE*>(messageData);
        std::cout << "[YARA] Malware match: " << rule->identifier << " in file: " << ctx->path << std::endl;
        ctx->matchFound = true;
    }
    return CALLBACK_CONTINUE;
}

void Yara::yaraCheck(const std::string& path) {
    Quarantine quarantine;

    std::ifstream file(path);
    if (!file.good()) {
        std::cerr << "[YARA] File not accessible: " << path << std::endl;
        return;
    }
    file.close();

    if (yr_initialize() != ERROR_SUCCESS) {
        std::cerr << "[YARA] Failed to initialize YARA." << std::endl;
        return;
    }

    YR_RULES* rules = nullptr;
    const char* rulePath = "D:\\Softwares\\Proj\\masha\\core\\malware.yarc";
    int loadResult = yr_rules_load(rulePath, &rules);
    if (loadResult != ERROR_SUCCESS || rules == nullptr) {
        std::cerr << "[YARA] Unable to load compiled rules: " << rulePath << " (error " << loadResult << ")" << std::endl;
        yr_finalize();
        return;
    }

    CallbackContext ctx{ path, &quarantine };
    int scanResult = yr_rules_scan_file(rules, path.c_str(), 0, yaraCallback, &ctx, 0);

    if (scanResult != ERROR_SUCCESS) {
        std::cerr << "[YARA] Failed to scan file: " << path << " (error " << scanResult << ")" << std::endl;
    }

    if (ctx.matchFound) {
        ctx.quarantine->quarantineMalware(path);
    }

    yr_rules_destroy(rules);
    yr_finalize();
}
