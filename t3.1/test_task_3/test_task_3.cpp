// test_task_3.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <Windows.h>
#define drvPath L"\\Device\\passthru"

#define IOCTL_SET_COMMAND1 CTL_CODE(FILE_DEVICE_UNKNOWN, 0xF07, METHOD_BUFFERED, FILE_ANY_ACCESS)


int _tmain(int argc, _TCHAR* argv[])
{
    HANDLE hDevice = INVALID_HANDLE_VALUE;  // handle to the drive to be examined 
    BOOL bResult   = FALSE;                 // results flag

    hDevice = CreateFileW(drvPath,          // drive to open
                        (GENERIC_READ | GENERIC_WRITE),                // no access to the drive
                        FILE_SHARE_READ | // share mode
                        FILE_SHARE_WRITE, 
                        NULL,             // default security attributes
                        OPEN_EXISTING,    // disposition
                        0,                // file attributes
                        NULL);            // do not copy file attributes
    int error = GetLastError();
    LPDWORD bytesReturned;

    bResult = DeviceIoControl(
        hDevice, // Handle драйвера из функции CreateFile
        (DWORD) IOCTL_SET_COMMAND1, // Комманда
        (LPVOID)0,
        (DWORD)0,
        (LPVOID)NULL,
        (DWORD)0,
        (LPDWORD)&bytesReturned,
        NULL
        );
    printf("%d\n",bResult);
    ERROR_ACCESS_DENIED;
    getchar();
	return 0;
}

