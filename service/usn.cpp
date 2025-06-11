#include <iostream>
#include <string>
#include <vector>
#include <windows.h>

#pragma pack(push, 1)
#include <winioctl.h>
#pragma pack(pop)

#include "usn.h"
#include "log.hpp"

Log s_log;

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

    HANDLE hFile = OpenFileById(
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
    DWORD pathLen = GetFinalPathNameByHandleW(hFile, pathBuffer, MAX_PATH, FILE_NAME_NORMALIZED);

    CloseHandle(hFile);

    if (pathLen == 0 || pathLen >= MAX_PATH)
        return L"";

    return std::wstring(pathBuffer);
}

void Usn::monitorDrive(HANDLE hServiceStopEvent) {
    HANDLE hVolume = CreateFileW(
        L"\\\\.\\D:",
        GENERIC_READ | GENERIC_WRITE,
        FILE_SHARE_READ | FILE_SHARE_WRITE,
        NULL,
        OPEN_EXISTING,
        0,
        NULL
    );

    if (hVolume == INVALID_HANDLE_VALUE) {
        s_log.logService("Failed to open handle to D: drive. Error: " + std::to_string(GetLastError()));
        return;
    }
    s_log.logService("Successfully opened handle to D: drive.");

    USN_JOURNAL_DATA jData = {0};
    DWORD bytesReturned;

    auto queryJournal = [&]() -> bool {
        return DeviceIoControl(
            hVolume,
            FSCTL_QUERY_USN_JOURNAL,
            NULL,
            0,
            &jData,
            sizeof(jData),
            &bytesReturned,
            NULL
        );
    };

    if (!queryJournal()) {
        s_log.logService("USN journal query failed. Error: " + std::to_string(GetLastError()));
        CloseHandle(hVolume);
        return;
    }

    USN nextUsn = jData.NextUsn;
    s_log.logService("USN journal queried successfully. NextUsn: " + std::to_string(nextUsn));

    std::vector<char> buffer(1024 * 1024); // 1MB buffer

    while (true) {
        if (WaitForSingleObject(hServiceStopEvent, 0) == WAIT_OBJECT_0) {
            s_log.logService("MonitorDrive: Stop event signaled. Exiting.");
            break;
        }

        READ_USN_JOURNAL_DATA rData = {0};
        rData.StartUsn = nextUsn;
        rData.ReasonMask = 0xFFFFFFFF;
        rData.ReturnOnlyOnClose = FALSE;
        rData.Timeout = 0;
        rData.BytesToWaitFor = 0;
        rData.UsnJournalID = jData.UsnJournalID;

        if (!DeviceIoControl(
                hVolume,
                FSCTL_READ_USN_JOURNAL,
                &rData,
                sizeof(rData),
                buffer.data(),
                (DWORD)buffer.size(),
                &bytesReturned,
                NULL
            )) {
            DWORD err = GetLastError();
            if (err == ERROR_JOURNAL_DELETE_IN_PROGRESS || err == ERROR_INVALID_PARAMETER) {
                s_log.logService("USN journal deleted or invalid. Re-querying journal...");
                if (!queryJournal()) {
                    s_log.logService("Re-query failed. Error: " + std::to_string(GetLastError()));
                    break;
                }
                nextUsn = jData.NextUsn;
                continue;
            }
            s_log.logService("FSCTL_READ_USN_JOURNAL failed. Error: " + std::to_string(err));
            break;
        }

        if (bytesReturned <= sizeof(USN)) {
            Sleep(1000);
            continue;
        }

        DWORD dwRetBytes = bytesReturned - sizeof(USN);
        char* ptr = buffer.data() + sizeof(USN);

        while (dwRetBytes > 0) {
            USN_RECORD* record = (USN_RECORD*)ptr;

            if (record->Reason & (USN_REASON_FILE_CREATE | USN_REASON_DATA_OVERWRITE | USN_REASON_DATA_EXTEND | USN_REASON_FILE_DELETE)) {
                std::wstring fullPath = GetPathFromFrn(hVolume, record->FileReferenceNumber);
                if (!fullPath.empty()) {
                    std::string narrowPath = WStringToString(fullPath);
                    s_log.logService("[+] Changed File: " + narrowPath);
                }
            }

            nextUsn = record->Usn + 1;
            ptr += record->RecordLength;
            dwRetBytes -= record->RecordLength;
        }

        Sleep(500);
    }

    s_log.logService("Stopped monitoring D: drive.");
    CloseHandle(hVolume);
}
