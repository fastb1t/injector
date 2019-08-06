#include <windows.h>

// [Thread1]:
DWORD WINAPI Thread1(LPVOID lpObject)
{
    for (size_t i = 0; i < 3; i++)
    {
        MessageBox(NULL, "We are hacked!", "Oops...", MB_OK | MB_ICONINFORMATION | MB_TOPMOST);
        Sleep(2000);
    }
    ExitThread(1337);
}
// [/Thread1]


// [DllMain]:
BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID lpReserved)
{
    switch (dwReason)
    {
    case DLL_PROCESS_ATTACH:
    {
        DWORD dwThreadId = 0;
        HANDLE hThread = CreateThread(NULL, 0, Thread1, NULL, 0, &dwThreadId);
        if (hThread != INVALID_HANDLE_VALUE)
        {
            CloseHandle(hThread);
        }
    }
    break;

    case DLL_PROCESS_DETACH:
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
        break;
    }
    return TRUE;
}
// [/DllMain]
