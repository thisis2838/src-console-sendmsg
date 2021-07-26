#include <iostream>
#include <cstdio>
#include "windows.h"
#include <comdef.h>
#include <tlhelp32.h>
#include "RemoteOps.h"
#include <string>

FARPROC msgfuncptr;
HANDLE gameprocess;

using namespace std;

extern "C" __declspec(dllexport)
int init(const char* procname)
{
    PROCESSENTRY32 entry;
    entry.dwSize = sizeof(PROCESSENTRY32);

    HANDLE snapshot = (HANDLE)CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, NULL);

    while (Process32Next(snapshot, &entry) == TRUE)
    {
        if (_stricmp(_bstr_t(entry.szExeFile), procname) == 0)
        {
            PVOID lpReturn = NULL;
            gameprocess = OpenProcess(PROCESS_ALL_ACCESS, false, entry.th32ProcessID);
            auto mod = GetRemoteModuleHandle(gameprocess, "tier0.dll");
            msgfuncptr = GetRemoteProcAddress(gameprocess, mod, "Msg");

            break;
        }
    }

    CloseHandle(snapshot);
}

extern "C" __declspec(dllexport)
void writemsg(const char* input)
{
    char d[2000];
    strcpy(d, input);

    if (msgfuncptr != nullptr && gameprocess != nullptr)
    {
        LPVOID dereercomp = VirtualAllocEx(gameprocess, NULL, strlen(d), MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
        if (dereercomp != 0)
        {
            WriteProcessMemory(gameprocess, dereercomp, d, strlen(d), NULL);
            auto s = CreateRemoteThread(gameprocess, NULL, NULL, (LPTHREAD_START_ROUTINE)msgfuncptr, dereercomp, 0, 0);

            if (s != 0)
            {
                WaitForSingleObject(s, INFINITE);
                VirtualFreeEx(gameprocess, dereercomp, strlen(d), MEM_RELEASE);
                TerminateThread(s, 0);
                CloseHandle(s);
            }
        }
    }
}

int main()
{
    return 0;
}