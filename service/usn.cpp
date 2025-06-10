#include <iostream>
#include <string>
#include <windows.h>
#include <winioctl.h>
#include "usn.h"
#include "log.hpp"

Log s_log;

// Function to convert std::wstring to std::string using UTF-8 encoding
std::string WStringToString(const std::wstring& wstr) {
    if (wstr.empty()) return std::string();

    int size_needed = WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), (int)wstr.length(), NULL, 0, NULL, NULL);
    std::string str_to(size_needed, 0);
    WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), (int)wstr.length(), &str_to[0], size_needed, NULL, NULL);

    return str_to;
}

std::wstring GetPathFromFrn(HANDLE hVolume, ULONGLONG frn) {
    FILE_ID_DESCRIPTOR fileIdDesc;
    fileIdDesc.dwSize = sizeof(FILE_ID_DESCRIPTOR);
    fileIdDesc.Type = FileIdType;
    fileIdDesc.FileId.QuadPart = frn;

    HANDLE hFile = OpenFileById(   /*File open handle*/
        hVolume,
        &fileIdDesc,
        0,
        FILE_SHARE_READ | FILE_SHARE_WRITE,
        NULL,
        0
    );

    if (hFile == INVALID_HANDLE_VALUE)
        return L"";

    wchar_t pathBuffer[MAX_PATH];
    DWORD pathLen = GetFinalPathNameByHandleW(
        hFile,
        pathBuffer,
        MAX_PATH,
        FILE_NAME_NORMALIZED
    );

    CloseHandle(hFile);

    if (pathLen == 0 || pathLen >= MAX_PATH)
        return L"";

    return std::wstring(pathBuffer);
}

void Usn::monitorDrive(HANDLE hServiceStopEvent) {
    HANDLE hVolume = CreateFileW(   /*file open handle*/
        L"\\\\.\\D:",
        GENERIC_READ | GENERIC_WRITE,
        FILE_SHARE_READ | FILE_SHARE_WRITE,
        NULL,
        OPEN_EXISTING,
        0,
        NULL
    );

    if (hVolume == INVALID_HANDLE_VALUE) {   /*Checking if handle worked*/
        s_log.logService("Failed to open handle to C: drive. Error: " + std::to_string(GetLastError()));
        return;
    }
    s_log.logService("Successfully opened handle to C: drive.");

    USN_JOURNAL_DATA jData = {0};   /*Setting up the variable for storing usn info aka query*/
    DWORD bytesReturned;

    if (!DeviceIoControl(hVolume, FSCTL_QUERY_USN_JOURNAL, NULL, 0, &jData, sizeof(jData), &bytesReturned, NULL)) {   /*Getting information about query*/
        s_log.logService("USN query failed. Error: " + std::to_string(GetLastError()));
        CloseHandle(hVolume);
        return;
    }
    s_log.logService("USN journal queried successfully. NextUsn: " + std::to_string(jData.NextUsn));


    USN nextUsn = jData.NextUsn;   /*initialing variable for the next usn data */

    while (true) {
        if (WaitForSingleObject(hServiceStopEvent, 0) == WAIT_OBJECT_0) {
            s_log.logService("MonitorDrive: Stop event signaled. Exiting monitoring loop.");
            break;
        }

        char buffer[4096];
        READ_USN_JOURNAL_DATA rData = {0};
        rData.StartUsn = nextUsn;   /*Variable for reading the new usn */
        rData.ReasonMask = 0xFFFFFFFF;
        rData.ReturnOnlyOnClose = FALSE;
        rData.Timeout = 0;
        rData.BytesToWaitFor = 0;
        rData.UsnJournalID = jData.UsnJournalID;

        if (!DeviceIoControl(    /*Reading and storing*/
            hVolume,
            FSCTL_READ_USN_JOURNAL,
            &rData,
            sizeof(rData),
            buffer,
            sizeof(buffer),
            &bytesReturned,
            NULL
        )) {
            DWORD err = GetLastError();
            if (err == ERROR_JOURNAL_DELETE_IN_PROGRESS || err == ERROR_INVALID_PARAMETER) {   /*Error handling*/
                s_log.logService("USN journal deleted or invalid. Restarting monitoring...");
                s_log.logService(std::to_string(err));
                Sleep(1000);
                continue;
            }
            s_log.logService("Failed to read USN journal. Error: " + std::to_string(err));
            break;
        }

        if (bytesReturned <= sizeof(USN)) {
            Sleep(1000);
            continue;
        }

        USN_RECORD* record = nullptr;
        DWORD dwRetBytes = bytesReturned - sizeof(USN);
        char* ptr = buffer + sizeof(USN);

        while (dwRetBytes > 0) {   /*Parsing*/
            record = (USN_RECORD*)ptr;

            if (record->Reason & (USN_REASON_FILE_CREATE | USN_REASON_DATA_OVERWRITE | USN_REASON_DATA_EXTEND | USN_REASON_FILE_DELETE)) {
                std::wstring fullPath = GetPathFromFrn(hVolume, record->FileReferenceNumber);
                if (!fullPath.empty()) {
                    // This line was the cause of the warning.
                    // It's now replaced with a call to the new WStringToString helper function.
                    std::string narrowPath = WStringToString(fullPath);
                    s_log.logService("[+] Changed File: " + narrowPath);
                }
            }

            nextUsn = record->Usn + 1;   /*Iterating to next journal*/
            ptr += record->RecordLength;
            dwRetBytes -= record->RecordLength;
        }

        Sleep(500);
    }

    s_log.logService("Stopped monitoring C: drive.");
    CloseHandle(hVolume);
}