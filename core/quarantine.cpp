#include <iostream>
#include "quarantine.h"
#include <windows.h>
#include <shlobj.h>
#include <filesystem>

std::string getProgramDataFolder() {
    PWSTR path = NULL;
    HRESULT hr = SHGetKnownFolderPath(FOLDERID_ProgramData, 0, NULL, &path);
    std::string result;
    if (SUCCEEDED(hr)) {
        int size_needed = WideCharToMultiByte(CP_UTF8, 0, path, -1, NULL, 0, NULL, NULL);
        std::string strPath(size_needed, 0);
        WideCharToMultiByte(CP_UTF8, 0, path, -1, &strPath[0], size_needed, NULL, NULL);
        result = strPath;
    } else {
        std::cerr << "Failed to get ProgramData path\n";
    }
    if (path) CoTaskMemFree(path);
    return result;
}

void Quarantine::quarantineMalware(const std::string& path) {
    std::string quarantineDir = getProgramDataFolder() + "\\mashaav\\malware";

    // Create the directory if it doesn't exist
    std::error_code ec;
    std::filesystem::create_directories(quarantineDir, ec);
    if (ec) {
        std::cerr << "Failed to create quarantine directory: " << ec.message() << "\n";
        return;
    }

    std::filesystem::path fpath(path);
    std::string filename = fpath.filename().string(); // Ensure it's a string
    std::string destPath = quarantineDir + "\\" + filename;
    std::cout << destPath;
    if (MoveFileA(path.c_str(), destPath.c_str())) {
        std::cout << "File quarantined to: " << destPath << "\n";
    } else {
        std::cerr << "Failed to quarantine file. Check if admin rights is give." << "\n";
    }
}
