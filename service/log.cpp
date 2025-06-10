#include <fstream>
#include <string>
#include <chrono>   // For std::chrono
#include <iomanip>  // For std::put_time
#include <locale>   // Required for std::put_time with std::localtime
#include "log.hpp"
#include <mutex>
const std::string LOG_FILE_PATH = "C:\\service_log.log";
static std::mutex g_logMutex;

void Log::logService(const std::string& text){
    std::lock_guard<std::mutex> lock(g_logMutex);
     std::ofstream logFile(LOG_FILE_PATH, std::ios_base::app);
    if (logFile.is_open()) {
        logFile << text << std::endl; // Just the text and a newline
        logFile.close(); // Close the file after writing.
    } else {
        // If logging to the file fails, consider writing to the Windows Event Log
        // as a fallback for critical errors, as this won't be visible in a service.
    }

}