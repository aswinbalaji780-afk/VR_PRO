#include <windows.h>
#include <iostream>
#include <string>

int main(int argc, char** argv) {
    if (argc < 3) {
        std::cout << "Usage: injector.exe <PID> <DLL_PATH>\n";
        return 1;
    }
    
    DWORD pid = std::stoi(argv[1]);
    std::string dllPath = argv[2];
    
    // Get full absolute path to the DLL
    char absoluteDllPath[MAX_PATH];
    GetFullPathNameA(dllPath.c_str(), MAX_PATH, absoluteDllPath, nullptr);

    std::cout << "[*] Target PID: " << pid << "\n";
    std::cout << "[*] DLL Path: " << absoluteDllPath << "\n";

    // 1. Open the target process
    HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pid);
    if (!hProcess) {
        std::cout << "[!] Failed to open process. Are you running as Administrator?\n";
        return 1;
    }
    
    // 2. Allocate memory in the target process for the DLL path string
    void* pDllPath = VirtualAllocEx(hProcess, 0, strlen(absoluteDllPath) + 1, MEM_COMMIT, PAGE_READWRITE);
    if (!pDllPath) {
        std::cout << "[!] Failed to allocate memory in target process.\n";
        CloseHandle(hProcess);
        return 1;
    }

    // 3. Write the DLL path string into the allocated memory
    WriteProcessMemory(hProcess, pDllPath, (void*)absoluteDllPath, strlen(absoluteDllPath) + 1, 0);
    
    // 4. Create a remote thread that calls LoadLibraryA, passing it our DLL path string
    HMODULE hKernel32 = GetModuleHandleA("Kernel32.dll");
    void* pLoadLibrary = (void*)GetProcAddress(hKernel32, "LoadLibraryA");
    
    HANDLE hThread = CreateRemoteThread(hProcess, 0, 0, (LPTHREAD_START_ROUTINE)pLoadLibrary, pDllPath, 0, 0);
    if (!hThread) {
        std::cout << "[!] Failed to create remote thread.\n";
        VirtualFreeEx(hProcess, pDllPath, strlen(absoluteDllPath) + 1, MEM_RELEASE);
        CloseHandle(hProcess);
        return 1;
    }
    
    // 5. Wait for injection to complete and clean up
    std::cout << "[+] Injecting...\n";
    WaitForSingleObject(hThread, INFINITE);
    
    VirtualFreeEx(hProcess, pDllPath, strlen(absoluteDllPath) + 1, MEM_RELEASE);
    CloseHandle(hThread);
    CloseHandle(hProcess);
    
    std::cout << "[+] Successfully injected!\n";
    return 0;
}
