#include <windows.h>
#include <tchar.h>
#include "usn.h"
SERVICE_STATUS        g_ServiceStatus = {0};
SERVICE_STATUS_HANDLE g_StatusHandle = NULL;
HANDLE                g_ServiceStopEvent = INVALID_HANDLE_VALUE;

#define SERVICE_NAME  _T("MySampleService")

void WINAPI ServiceMain(DWORD argc, LPTSTR *argv);
void WINAPI ServiceCtrlHandler(DWORD CtrlCode);

int _tmain(int argc, TCHAR *argv[])
{
    SERVICE_TABLE_ENTRY ServiceTable[] =
    {
        { SERVICE_NAME, (LPSERVICE_MAIN_FUNCTION)ServiceMain },
        { NULL, NULL }
    };

    if (!StartServiceCtrlDispatcher(ServiceTable))
    {
        // Logging failed start
        return GetLastError();
    }

    return 0;
}

void WINAPI ServiceMain(DWORD argc, LPTSTR *argv)
{
    g_StatusHandle = RegisterServiceCtrlHandler(SERVICE_NAME, ServiceCtrlHandler);
    if (!g_StatusHandle) return;

    ZeroMemory(&g_ServiceStatus, sizeof(g_ServiceStatus));
    g_ServiceStatus.dwServiceType = SERVICE_WIN32_OWN_PROCESS;
    g_ServiceStatus.dwControlsAccepted = 0;
    g_ServiceStatus.dwCurrentState = SERVICE_START_PENDING;
    SetServiceStatus(g_StatusHandle, &g_ServiceStatus);

    // Create a stop event
    g_ServiceStopEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
    if (g_ServiceStopEvent == NULL)
    {
        g_ServiceStatus.dwControlsAccepted = 0;
        g_ServiceStatus.dwCurrentState = SERVICE_STOPPED;
        g_ServiceStatus.dwWin32ExitCode = GetLastError();
        SetServiceStatus(g_StatusHandle, &g_ServiceStatus);
        return;
    }

    g_ServiceStatus.dwControlsAccepted = SERVICE_ACCEPT_STOP;
    g_ServiceStatus.dwCurrentState = SERVICE_RUNNING;
    SetServiceStatus(g_StatusHandle, &g_ServiceStatus);

    // MAIN LOGIC GOES HERE
    Usn usn;
    while (WaitForSingleObject(g_ServiceStopEvent, 0) != WAIT_OBJECT_0)
    {
        usn.monitorDrive(g_ServiceStopEvent);
        Sleep(1000); // Simulate work
    }

    // Service is stopping
    g_ServiceStatus.dwControlsAccepted = 0;
    g_ServiceStatus.dwCurrentState = SERVICE_STOPPED;
    SetServiceStatus(g_StatusHandle, &g_ServiceStatus);
}

void WINAPI ServiceCtrlHandler(DWORD CtrlCode)
{
    switch (CtrlCode)
    {
    case SERVICE_CONTROL_STOP:
        if (g_ServiceStatus.dwCurrentState != SERVICE_RUNNING)
            break;

        g_ServiceStatus.dwControlsAccepted = 0;
        g_ServiceStatus.dwCurrentState = SERVICE_STOP_PENDING;
        SetServiceStatus(g_StatusHandle, &g_ServiceStatus);

        SetEvent(g_ServiceStopEvent);
        break;
    default:
        break;
    }
}
//myr thalla