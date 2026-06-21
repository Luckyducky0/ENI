#include <windows.h>
#include <tlhelp32.h>
#include <iostream>
#include <string>

DWORD FindRobloxPID() {
    HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    PROCESSENTRY32W pe;
    pe.dwSize = sizeof(pe);
    DWORD pid = 0;

    if (Process32FirstW(snap, &pe)) {
        do {
            if (wcscmp(pe.szExeFile, L"RobloxPlayerBeta.exe") == 0) {
                pid = pe.th32ProcessID;
                break;
            }
        } while (Process32NextW(snap, &pe));
    }
    CloseHandle(snap);
    return pid;
}

bool FileExists(const char* path) {
    DWORD attrib = GetFileAttributesA(path);
    return (attrib != INVALID_FILE_ATTRIBUTES &&
        !(attrib & FILE_ATTRIBUTE_DIRECTORY));
}

bool InjectDLL(DWORD pid, const char* dllPath) {
    HANDLE hProc = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pid);
    if (!hProc) {
        std::cerr << "[!] Failed to open process. Error: "
            << GetLastError() << "\n";
        std::cerr << "[!] Try running as Administrator.\n";
        return false;
    }

    if (!FileExists(dllPath)) {
        std::cerr << "[!] DLL not found at: " << dllPath << "\n";
        CloseHandle(hProc);
        return false;
    }

    size_t pathLen = strlen(dllPath) + 1;
    LPVOID remoteMem = VirtualAllocEx(hProc, NULL, pathLen,
        MEM_COMMIT | MEM_RESERVE,
        PAGE_READWRITE);
    if (!remoteMem) {
        std::cerr << "[!] VirtualAllocEx failed. Error: "
            << GetLastError() << "\n";
        CloseHandle(hProc);
        return false;
    }

    if (!WriteProcessMemory(hProc, remoteMem, dllPath, pathLen, NULL)) {
        std::cerr << "[!] WriteProcessMemory failed. Error: "
            << GetLastError() << "\n";
        VirtualFreeEx(hProc, remoteMem, 0, MEM_RELEASE);
        CloseHandle(hProc);
        return false;
    }

    FARPROC loadLib = GetProcAddress(
        GetModuleHandleA("kernel32.dll"), "LoadLibraryA"
    );

    HANDLE hThread = CreateRemoteThread(
        hProc, NULL, 0,
        (LPTHREAD_START_ROUTINE)loadLib,
        remoteMem, 0, NULL
    );

    if (!hThread) {
        std::cerr << "[!] CreateRemoteThread failed. Error: "
            << GetLastError() << "\n";
        VirtualFreeEx(hProc, remoteMem, 0, MEM_RELEASE);
        CloseHandle(hProc);
        return false;
    }

    std::cout << "[*] Waiting for injection to complete...\n";
    WaitForSingleObject(hThread, 10000);

    DWORD exitCode = 0;
    GetExitCodeThread(hThread, &exitCode);

    VirtualFreeEx(hProc, remoteMem, 0, MEM_RELEASE);
    CloseHandle(hThread);
    CloseHandle(hProc);

    if (exitCode == 0) {
        std::cerr << "[!] DLL load returned NULL — injection may have failed.\n";
        return false;
    }

    std::cout << "[+] DLL injected successfully. Module base: 0x"
        << std::hex << exitCode << "\n";
    return true;
}

int main() {
    std::cout << "\n  =============================\n";
    std::cout << "       ENI Executor v1.0\n";
    std::cout << "       coded with love <3\n";
    std::cout << "  =============================\n\n";

    std::cout << "[*] Scanning for Roblox process...\n";

    DWORD pid = FindRobloxPID();
    if (!pid) {
        std::cerr << "[!] RobloxPlayerBeta.exe not found.\n";
        std::cerr << "[!] Launch Roblox and join a game first.\n";
        std::cout << "\nPress Enter to exit...";
        std::cin.get();
        return 1;
    }

    std::cout << "[+] Found Roblox — PID: " << pid << "\n";

    char exePath[MAX_PATH];
    GetModuleFileNameA(NULL, exePath, MAX_PATH);

    // Strip the exe filename to get the folder path
    std::string basePath(exePath);
    size_t lastSlash = basePath.find_last_of("\\");
    if (lastSlash != std::string::npos) {
        basePath = basePath.substr(0, lastSlash);
    }

    std::string dllPath = basePath + "\\payload.dll";

    std::cout << "[*] DLL path: " << dllPath << "\n";
    std::cout << "[*] Injecting...\n";

    if (InjectDLL(pid, dllPath.c_str())) {
        std::cout << "[+] Done! Check Roblox for the executor console.\n";
    }
    else {
        std::cerr << "[!] Injection failed. See errors above.\n";
    }

    std::cout << "\nPress Enter to exit...";
    std::cin.get();
    return 0;
}