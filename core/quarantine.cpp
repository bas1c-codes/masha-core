#include "quarantine.h"
#include <windows.h>
#include <shlobj.h>
#include <iostream>
#include <filesystem>
#include <chrono>
#include "encrypt.h"
std::string getLocalAppDataFolder() {
    PWSTR path = NULL;
    std::string result;

    HRESULT hr = SHGetKnownFolderPath(FOLDERID_LocalAppData, 0, NULL, &path);
    if (SUCCEEDED(hr)) {
        // Convert wchar_t* to UTF-8 std::string
        int size_needed = WideCharToMultiByte(CP_UTF8, 0, path, -1, NULL, 0, NULL, NULL);
        if (size_needed > 0) {
            std::string strPath(size_needed, 0);
            WideCharToMultiByte(CP_UTF8, 0, path, -1, &strPath[0], size_needed, NULL, NULL);
            strPath.resize(size_needed - 1); // Remove null terminator
            result = strPath;
        }
    } else {
        std::cerr << "Failed to get LocalAppData path\n";
    }

    if (path) CoTaskMemFree(path);
    return result;
}

void Quarantine::quarantineMalware(const std::string& originalPath) {
    namespace fs = std::filesystem;

    std::string quarantineDir = getLocalAppDataFolder() + "\\mashaav\\malware";
    fs::create_directories(quarantineDir); // ensure directory exists
    fs::path fpath(originalPath);
    std::string filename = fpath.filename().string();

    // Add timestamp to filename to avoid duplicates
    auto timestamp = std::chrono::system_clock::now().time_since_epoch().count();
    std::string uniqueFilename = filename + "_" + std::to_string(timestamp);
    std::string destPath = quarantineDir + "\\" + uniqueFilename;
    if (MoveFileA(originalPath.c_str(), destPath.c_str())) {
        Encrypt encrypt;
        encrypt.encryptFile(destPath);
        std::cout << "File quarantined to: " << destPath << "\n";
    } else {
        DWORD err = GetLastError();
        std::cerr << "Failed to quarantine file. Error code: " << err << "\n";
        if (err == 5)
            std::cerr << "Access denied. Try running as administrator.\n";
        else if (err == 183)
            std::cerr << "File already exists at destination.\n";
    }
}
