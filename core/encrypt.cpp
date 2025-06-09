#include <string>
#include "encrypt.h"
#include <windows.h>
#include <fstream>
#include <iostream>
#include <vector>

void Encrypt::encryptFile(const std::string& filePath) {
    try {
        std::ifstream file(filePath, std::ios::binary);
        if (!file) {
            std::cerr << "Error opening file: " << filePath << "\n";
            return;
        }

        std::vector<char> buffer((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
        file.close();

        if (buffer.empty()) {
            std::cerr << "File is empty or failed to read: " << filePath << "\n";
            return;
        }

        DATA_BLOB dataIn;
        dataIn.cbData = static_cast<DWORD>(buffer.size());
        dataIn.pbData = reinterpret_cast<BYTE*>(buffer.data());

        DATA_BLOB dataOut;

        // Encrypt using DPAPI
        if (!CryptProtectData(&dataIn, nullptr, nullptr, nullptr, nullptr, 0, &dataOut)) {
            std::cerr << "Could not encrypt the file. Error: " << GetLastError() << "\n";
            return;
        }

        std::ofstream outFile(filePath, std::ios::binary | std::ios::trunc);
        if (!outFile) {
            std::cerr << "Failed to open file for writing: " << filePath << "\n";
            LocalFree(dataOut.pbData);
            return;
        }

        outFile.write(reinterpret_cast<char*>(dataOut.pbData), dataOut.cbData);
        outFile.close();
        LocalFree(dataOut.pbData);

        std::cout << "Data encrypted successfully: " << filePath << "\n";
    }
    catch (const std::exception& ex) {
        std::cerr << "Exception occurred: " << ex.what() << "\n";
    }
    catch (...) {
        std::cerr << "Unknown error occurred during encryption.\n";
    }
}
