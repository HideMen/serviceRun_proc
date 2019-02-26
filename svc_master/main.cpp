#include <windows.h>
#include <tchar.h>
#include <strsafe.h>
#include <array>
#include <string>

#pragma comment(lib, "advapi32.lib")

#define SVCNAME TEXT("MicrosoftAlphaVite4")

SERVICE_STATUS gSvcStatus;
SERVICE_STATUS_HANDLE gSvcStatusHandle;
HANDLE ghSvcStopEvent = NULL;

VOID SvcInstall(void);
VOID SvcUninstall(void);
VOID SvcWork(void);
VOID WINAPI SvcCtrlHandler(DWORD);
VOID WINAPI SvcMain(DWORD, LPTSTR *);

VOID ReportSvcStatus(DWORD, DWORD, DWORD);
VOID SvcInit(DWORD, LPTSTR *);

void __cdecl _tmain(int argc, TCHAR *argv[])
{
    if (lstrcmpi(argv[1], TEXT("install")) == 0)
    {
        SvcInstall();
        return;
    }
    else if (lstrcmpi(argv[1], TEXT("uninstall")) == 0)
    {
        SvcUninstall();
        return;
    }

    SERVICE_TABLE_ENTRY DispatchTable[] =
    {
        { SVCNAME, (LPSERVICE_MAIN_FUNCTION)SvcMain },
        { NULL, NULL }
    };


    if (!StartServiceCtrlDispatcher(DispatchTable))
    {
    }
}

VOID SvcInstall()
{
    SC_HANDLE schSCManager;
    SC_HANDLE schService;

    TCHAR szPath[MAX_PATH];
    if (!GetModuleFileName(NULL, szPath, MAX_PATH))
    {
        return;
    }

    schSCManager = OpenSCManager(
        NULL,                  
        NULL,                 
        SC_MANAGER_ALL_ACCESS); 

    if (NULL == schSCManager)
    {
        return;
    }

    schService = CreateService(
        schSCManager,              // SCM database 
        SVCNAME,                   // name of service 
        SVCNAME,                   // service name to display 
        SERVICE_ALL_ACCESS,         // desired access 
        SERVICE_WIN32_OWN_PROCESS | SERVICE_INTERACTIVE_PROCESS, // service type 
        SERVICE_AUTO_START,       // start type 
        SERVICE_ERROR_NORMAL,       // error control type 
        szPath,                    // path to service's binary 
        NULL,                      // no load ordering group 
        NULL,                      // no tag identifier 
        NULL,                      // no dependencies 
        NULL,                      // LocalSystem account 
        NULL);                     // no password 

    if (schService == NULL)
    {
        CloseServiceHandle(schSCManager);
        return;
    }

    CloseServiceHandle(schService);
    CloseServiceHandle(schSCManager);
}


VOID SvcUninstall(void)
{
    SC_HANDLE schSCManager;
    SC_HANDLE schService;

    schSCManager = OpenSCManager(
        NULL,               
        NULL,                    
        SC_MANAGER_ALL_ACCESS);  

    if (NULL == schSCManager)
    {
        return;
    }

    schService = OpenService(
        schSCManager, 
        SVCNAME,       
        DELETE);    

    if (schService == NULL)
    {
        CloseServiceHandle(schSCManager);
        return;
    }

	if (!DeleteService(schService));

    CloseServiceHandle(schService);
    CloseServiceHandle(schSCManager);
}

VOID WINAPI SvcMain(DWORD dwArgc, LPTSTR *lpszArgv)
{
    gSvcStatusHandle = RegisterServiceCtrlHandler(SVCNAME, SvcCtrlHandler);
    if (!gSvcStatusHandle)
    {
        return;
    }

	gSvcStatus.dwServiceType = SERVICE_WIN32_OWN_PROCESS;
    gSvcStatus.dwServiceSpecificExitCode = 0;

    ReportSvcStatus(SERVICE_START_PENDING, NO_ERROR, 3000);

    SvcInit(dwArgc, lpszArgv);
}

VOID SvcInit(DWORD dwArgc, LPTSTR *lpszArgv)
{
    ghSvcStopEvent = CreateEvent(
        NULL,    
        TRUE,  
        FALSE, 
        NULL); 

    if (ghSvcStopEvent == NULL)
    {
        ReportSvcStatus(SERVICE_STOPPED, NO_ERROR, 0);
        return;
    }

    ReportSvcStatus(SERVICE_RUNNING, NO_ERROR, 0);

    while (TRUE)
    {
        SvcWork();

        WaitForSingleObject(ghSvcStopEvent, INFINITE);

        ReportSvcStatus(SERVICE_STOPPED, NO_ERROR, 0);
        return;
    }
}

VOID ReportSvcStatus(DWORD dwCurrentState, DWORD dwWin32ExitCode, DWORD dwWaitHint)
{
    static DWORD dwCheckPoint = 1;


    gSvcStatus.dwCurrentState = dwCurrentState;
    gSvcStatus.dwWin32ExitCode = dwWin32ExitCode;
    gSvcStatus.dwWaitHint = dwWaitHint;

    if (dwCurrentState == SERVICE_START_PENDING)
        gSvcStatus.dwControlsAccepted = 0;
    else gSvcStatus.dwControlsAccepted = SERVICE_ACCEPT_STOP;

    if ((dwCurrentState == SERVICE_RUNNING) || (dwCurrentState == SERVICE_STOPPED))
        gSvcStatus.dwCheckPoint = 0;
    else gSvcStatus.dwCheckPoint = dwCheckPoint++;

    SetServiceStatus(gSvcStatusHandle, &gSvcStatus);
}

VOID WINAPI SvcCtrlHandler(DWORD dwCtrl)
{

    switch (dwCtrl)
    {
    case SERVICE_CONTROL_STOP:
        ReportSvcStatus(SERVICE_STOP_PENDING, NO_ERROR, 0);


        SetEvent(ghSvcStopEvent);
        ReportSvcStatus(gSvcStatus.dwCurrentState, NO_ERROR, 0);

        return;

    case SERVICE_CONTROL_INTERROGATE:
        break;

    default:
        break;
    }
}
