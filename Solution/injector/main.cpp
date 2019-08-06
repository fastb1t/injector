#include <iostream>
#include <windows.h>
#include <tlhelp32.h>

// [FileIsExist]:
bool FileIsExist(const char* szFileName)
{
    if (szFileName == nullptr || szFileName[0] == 0)
    {
        return false;
    }

    WIN32_FIND_DATA wfd;
    HANDLE hFile = FindFirstFile(szFileName, &wfd);
    if (hFile != INVALID_HANDLE_VALUE)
    {
        FindClose(hFile);
        return true;
    }
    return false;
}
// [/FileIsExist]


// [ProcessIsExist]:
bool ProcessIsExist(DWORD dwPID)
{
    if (dwPID == 0)
    {
        return false;
    }

    HANDLE hProcess = OpenProcess(SYNCHRONIZE, FALSE, dwPID);
    if (hProcess != INVALID_HANDLE_VALUE)
    {
        if (WaitForSingleObject(hProcess, 0) == WAIT_TIMEOUT)
        {
            CloseHandle(hProcess);
            return true;
        }
        CloseHandle(hProcess);
        return false;
    }
    return false;
}
// [/ProcessIsExist]


// [GetPID]:
DWORD GetPID(const char* szProcessName)
{
    if (szProcessName == nullptr || szProcessName[0] == 0)
    {
        return 0;
    }

    DWORD dwProcessId = 0;

    PROCESSENTRY32 pe32;
    memset(&pe32, 0, sizeof(PROCESSENTRY32));
    pe32.dwSize = sizeof(PROCESSENTRY32);

    HANDLE hProcessSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hProcessSnap == INVALID_HANDLE_VALUE)
    {
        return 0;
    }

    if (!Process32First(hProcessSnap, &pe32))
    {
        if (GetLastError() == ERROR_NO_MORE_FILES)
        {
            CloseHandle(hProcessSnap);
            return 0;
        }
    }

    do
    {
        if (!lstrcmp(pe32.szExeFile, szProcessName))
        {
            dwProcessId = pe32.th32ProcessID;
            break;
        }
    } while (Process32Next(hProcessSnap, &pe32));

    CloseHandle(hProcessSnap);
    return dwProcessId;
}
// [/GetPID]


// [InjectDll]:
static bool InjectDll(const char* szDllName, DWORD dwProcessId)
{
    if (szDllName == nullptr ||
        szDllName[0] == 0 ||
        dwProcessId == 0)
    {
        return false;
    }

    HANDLE hProcess = OpenProcess(
        PROCESS_CREATE_THREAD | PROCESS_QUERY_INFORMATION | PROCESS_VM_OPERATION | PROCESS_VM_READ | PROCESS_VM_WRITE,
        FALSE,
        dwProcessId
    );
    if (!hProcess)
    {
        return false;
    }

    HMODULE hKernel32 = GetModuleHandleA("kernel32.dll");
    if (!hKernel32)
    {
        return false;
    }

    LPVOID lpLoadLibraryAddress = (LPVOID)GetProcAddress(hKernel32, "LoadLibraryA");
    if (!lpLoadLibraryAddress)
    {
        return false;
    }

    try
    {
        LPVOID lpRemoteString = (LPVOID)VirtualAllocEx(
            hProcess,
            NULL,
            static_cast<SIZE_T>(lstrlen(szDllName)) + 1,
            MEM_RESERVE | MEM_COMMIT,
            PAGE_READWRITE
        );
        if (lpRemoteString == nullptr)
        {
            throw std::exception("VirtualAllocEx failed.");
        }

        if (!WriteProcessMemory(
            hProcess,
            lpRemoteString,
            (LPVOID)szDllName,
            static_cast<SIZE_T>(lstrlen(szDllName)) + 1,
            NULL
        ))
        {
            throw std::exception("WriteProcessMemory failed.");
        }

        if (!CreateRemoteThread(
            hProcess,
            NULL,
            NULL,
            (LPTHREAD_START_ROUTINE)lpLoadLibraryAddress,
            lpRemoteString,
            NULL,
            NULL
        ))
        {
            throw std::exception("CreateRemoteThread failed.");
        }
    }
    catch (std::exception &e)
    {
        std::cerr << e.what() << "\n";
    }

    CloseHandle(hProcess);
    return true;
}
// [/InjectDll]


// [main]:
int main(int argc, char* argv[])
{
    std::string dll_name;

    DWORD dwPID = 0;
    std::string process_name;

    enum {
        WRONG_MODE,
        USING_PID,
        USING_PROCESS_NAME
    } mode = WRONG_MODE;

    if (argc != 5)
    {
        std::cerr
            << "\n Usage:"
            << "\n   --dll [path to dll file]"
            << "\n   --pid [process id]"
            << "\n      or"
            << "\n   --process-name [process name]"
            << "\n"
            << "\n For example:"
            << "\n   injector.exe --dll TestDll.dll --pid 7040"
            << "\n   injector.exe --dll TestDll.dll --process-name TestProgram.exe"
            << "\n";
        return EXIT_FAILURE;
    }

    std::cout << "\n";

    for (int i = 1; i < argc; i++)
    {
        if (!lstrcmp(argv[i], "--dll") && (i + 1) < argc)
        {
            dll_name = argv[i + 1];
            i++;

            if (dll_name.empty())
            {
                std::cerr << "[ - ] Syntax error.\n";
                return EXIT_FAILURE;
            }

            if (!FileIsExist(dll_name.c_str()))
            {
                std::cerr << "[ - ] File '" << dll_name << "' not found.\n";
                return EXIT_FAILURE;
            }
        }

        if (!lstrcmp(argv[i], "--pid") && (i + 1) < argc && mode == WRONG_MODE)
        {
            dwPID = atoi(argv[i + 1]);
            i++;

            if (dwPID > 0)
            {
                mode = USING_PID;

                if (!ProcessIsExist(dwPID))
                {
                    std::cerr << "[ - ] Process with this id '" << dwPID << "' not found.\n";
                    return EXIT_FAILURE;
                }
            }
            else
            {
                std::cerr << "[ - ] Syntax error.\n";
                return EXIT_FAILURE;
            }
        }

        if (!lstrcmp(argv[i], "--process-name") && (i + 1) < argc && mode == WRONG_MODE)
        {
            process_name = argv[i + 1];
            i++;

            if (!process_name.empty())
            {
                mode = USING_PROCESS_NAME;

                if (!(dwPID = GetPID(process_name.c_str())))
                {
                    std::cerr << "[ - ] Process '" << process_name << "' not found.\n";
                    return EXIT_FAILURE;
                }
            }
            else
            {
                std::cerr << "[ - ] Syntax error.\n";
                return EXIT_FAILURE;
            }
        }
    }

    if (mode == WRONG_MODE || dll_name.empty())
    {
        std::cerr << "[ - ] Syntax error.\n";
        return EXIT_FAILURE;
    }

    std::cout << "Process ID:   " << dwPID << "\n";
    std::cout << "DLL:          " << dll_name << "\n";
    std::cout << "\n";

    if (!InjectDll(dll_name.c_str(), dwPID))
    {
        std::cerr << "[ - ] Inject failed\n";
        return EXIT_FAILURE;
    }

    std::cerr << "[ + ] Injected\n";
    return EXIT_SUCCESS;
}
// [/main]
