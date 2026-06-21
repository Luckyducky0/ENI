#include <windows.h>
#include <tlhelp32.h>
#include <string>

DWORD GetRobloxPID();
bool InjectDLL(DWORD pid, const std::string& dllPath);

HWND hEdit;
std::string dllPath = "payload.dll";

void ExecuteFromGUI() {
    char buffer[65536] = { 0 };
    GetWindowTextA(hEdit, buffer, 65536);

    if (strlen(buffer) == 0) {
        MessageBoxA(NULL, "Paste script first!", "ENI", MB_OK | MB_ICONWARNING);
        return;
    }

    DWORD pid = GetRobloxPID();
    if (pid == 0) {
        MessageBoxA(NULL, "Open Roblox first!", "ENI", MB_OK | MB_ICONERROR);
        return;
    }

    if (InjectDLL(pid, dllPath)) {
        MessageBoxA(NULL, "Injected successfully!", "ENI", MB_OK | MB_ICONINFORMATION);
    }
    else {
        MessageBoxA(NULL, "Injection failed. Run as Administrator.", "ENI", MB_OK | MB_ICONERROR);
    }
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
    case WM_CREATE:
        CreateWindowA("STATIC", "ENI Roblox External Injector", WS_VISIBLE | WS_CHILD, 10, 10, 400, 25, hwnd, NULL, NULL, NULL);
        hEdit = CreateWindowA("EDIT", "", WS_VISIBLE | WS_CHILD | WS_BORDER | ES_MULTILINE | ES_AUTOVSCROLL | WS_VSCROLL, 10, 40, 400, 180, hwnd, NULL, NULL, NULL);
        CreateWindowA("BUTTON", "Inject + Run Script", WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON, 10, 230, 400, 50, hwnd, (HMENU)1, NULL, NULL);
        break;

    case WM_COMMAND:
        if (LOWORD(wParam) == 1) ExecuteFromGUI();
        break;

    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    }
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int nCmdShow) {
    const char CLASS_NAME[] = "ENIClass";

    WNDCLASSA wc = { 0 };
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = CLASS_NAME;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);

    RegisterClassA(&wc);

    HWND hwnd = CreateWindowA(CLASS_NAME, "ENI Injector", WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, 440, 340, NULL, NULL, hInstance, NULL);

    ShowWindow(hwnd, nCmdShow);
    UpdateWindow(hwnd);

    MSG msg = {};
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    return 0;
}

DWORD GetRobloxPID() {
    HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hSnapshot == INVALID_HANDLE_VALUE) return 0;

    PROCESSENTRY32 pe32 = { 0 };
    pe32.dwSize = sizeof(PROCESSENTRY32);

    if (Process32First(hSnapshot, &pe32)) {
        do {
            char exeName[260] = { 0 };
            WideCharToMultiByte(CP_ACP, 0, pe32.szExeFile, -1, exeName, 260, NULL, NULL);
            if (_stricmp(exeName, "RobloxPlayerBeta.exe") == 0) {
                CloseHandle(hSnapshot);
                return pe32.th32ProcessID;
            }
        } while (Process32Next(hSnapshot, &pe32));
    }
    CloseHandle(hSnapshot);
    return 0;
}

bool InjectDLL(DWORD pid, const std::string& dllPath) {
    HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pid);
    if (!hProcess) return false;

    SIZE_T len = dllPath.length() + 1;
    LPVOID pRemote = VirtualAllocEx(hProcess, NULL, len, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
    if (!pRemote) {
        CloseHandle(hProcess);
        return false;
    }

    WriteProcessMemory(hProcess, pRemote, dllPath.c_str(), len, NULL);

    HMODULE hKernel = GetModuleHandleA("kernel32.dll");
    FARPROC pLoad = GetProcAddress(hKernel, "LoadLibraryA");

    HANDLE hThread = CreateRemoteThread(hProcess, NULL, 0, (LPTHREAD_START_ROUTINE)pLoad, pRemote, 0, NULL);
    if (hThread) {
        WaitForSingleObject(hThread, INFINITE);
        CloseHandle(hThread);
    }

    VirtualFreeEx(hProcess, pRemote, 0, MEM_RELEASE);
    CloseHandle(hProcess);
    return true;
}