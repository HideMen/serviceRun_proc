#include <windows.h>
#include <tchar.h>
#include <wtsapi32.h>
#include <userenv.h>
#include <stdio.h>
#include <direct.h>
#include <iostream>
#include <array>
#include <string>

#pragma comment(lib, "wtsapi32")
#pragma comment(lib, "userenv")


//функция поиска процесса
int GetProcessByName(const char* process_name)
{
	WTS_PROCESS_INFO* pWPIs = NULL;
	DWORD dwProcCount = 0;
	if (WTSEnumerateProcesses(WTS_CURRENT_SERVER_HANDLE, NULL, 1, &pWPIs, &dwProcCount))
	{
		//Go through all processes retrieved
		for (DWORD i = 0; i < dwProcCount; i++)
		{
			if (_stricmp(pWPIs[i].pProcessName, process_name) == 0)
			{
				return 1;
			}
			//pWPIs[i].pProcessName = process file name only, no path!
			//pWPIs[i].ProcessId = process ID
			//pWPIs[i].SessionId = session ID, if you need to limit it to the logged in user processes
			//pWPIs[i].pUserSid = user SID that started the process
		}
	}

	//Free memory
	if (pWPIs)
	{
		WTSFreeMemory(pWPIs);
		pWPIs = NULL;
	}
	return 0;
}
VOID SvcWork()
{
	std::string systemDrive = getenv("SystemDrive");
	systemDrive = systemDrive + "\\";
	_chdir(systemDrive.c_str());


	//выполняем цикл пока процесс не будет запущен

	HANDLE hToken = NULL;
	DWORD dwSessionId = WTSGetActiveConsoleSessionId();

	WTSQueryUserToken(dwSessionId, &hToken);

	while (true)
	{

		if (GetProcessByName("svc_worker.exe") == 0)
		{

			STARTUPINFO si;
			ZeroMemory(&si, sizeof(STARTUPINFO));
			si.cb = sizeof(STARTUPINFO);
			si.lpDesktop = TEXT("winsta0\\default");

			si.dwFlags = STARTF_USESHOWWINDOW;
			si.wShowWindow = SW_SHOW;

			PROCESS_INFORMATION pi;
			ZeroMemory(&pi, sizeof(PROCESS_INFORMATION));

			PVOID lpEnvironment = NULL;
			CreateEnvironmentBlock(&lpEnvironment, hToken, FALSE);

			LPTSTR szCmdline = _tcsdup(_T("\"svc_worker.exe\""));

			CreateProcessAsUser(hToken,
				NULL,
				szCmdline,
				NULL,
				NULL,
				FALSE,
				NORMAL_PRIORITY_CLASS | DETACHED_PROCESS | CREATE_UNICODE_ENVIRONMENT,
				lpEnvironment,
				NULL,
				&si,
				&pi);
			CloseHandle(pi.hProcess);
			CloseHandle(pi.hThread);
		}

		Sleep(5000);
	}
	CloseHandle(hToken);

}