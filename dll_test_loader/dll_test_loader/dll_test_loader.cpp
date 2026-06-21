#include <windows.h>
#include <iostream>
#include <string>

void PrintLastError() {
    DWORD error = GetLastError();

    char* messageBuffer = nullptr;

    FormatMessageA(
        FORMAT_MESSAGE_ALLOCATE_BUFFER |
        FORMAT_MESSAGE_FROM_SYSTEM |
        FORMAT_MESSAGE_IGNORE_INSERTS,
        NULL,
        error,
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        (LPSTR)&messageBuffer,
        0,
        NULL
    );

    std::cerr << "[!] Error code: " << error << "\n";

    if (messageBuffer) {
        std::cerr << "[!] Message: " << messageBuffer << "\n";
        LocalFree(messageBuffer);
    }
}

int main() {
    std::cout << "==============================\n";
    std::cout << "   DLL Test Loader\n";
    std::cout << "==============================\n\n";

    const char* dllName = "payload.dll";

    std::cout << "[*] Trying to load: " << dllName << "\n";

    HMODULE hDll = LoadLibraryA(dllName);

    if (!hDll) {
        std::cerr << "[!] Failed to load DLL.\n";
        PrintLastError();

        std::cout << "\nMake sure payload.dll is in the SAME folder as this .exe.\n";
        std::cout << "Press Enter to exit...";
        std::cin.get();
        return 1;
    }

    std::cout << "[+] DLL loaded successfully!\n";
    std::cout << "[*] Module handle: 0x" << std::hex << (uintptr_t)hDll << "\n\n";

    std::cout << "If your DLL creates a console or prints output, you should see it now.\n";
    std::cout << "Press Enter to unload the DLL and exit...";
    std::cin.get();

    FreeLibrary(hDll);

    std::cout << "[+] DLL unloaded.\n";
    return 0;
}