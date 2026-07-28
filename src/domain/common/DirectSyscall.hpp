#pragma once

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <Winternl.h>
#include <cstdint>

#pragma comment(lib, "ntdll.lib")

namespace nuub::domain::syscall {

// Windows 10/11 syscall numbers (x64)
// These change between Windows versions, so we detect at runtime
class SyscallResolver {
    HMODULE ntdll_ = nullptr;

    // Function pointers for resolved syscalls
    using NtAllocateVirtualMemory_t = NTSTATUS(NTAPI*)(
        HANDLE ProcessHandle,
        PVOID* BaseAddress,
        ULONG_PTR ZeroBits,
        PSIZE_T RegionSize,
        ULONG AllocationType,
        ULONG Protect);

    using NtWriteVirtualMemory_t = NTSTATUS(NTAPI*)(
        HANDLE ProcessHandle,
        PVOID BaseAddress,
        PVOID Buffer,
        SIZE_T NumberOfBytesToWrite,
        PSIZE_T NumberOfBytesWritten);

    using NtProtectVirtualMemory_t = NTSTATUS(NTAPI*)(
        HANDLE ProcessHandle,
        PVOID* BaseAddress,
        PSIZE_T RegionSize,
        ULONG NewProtect,
        PULONG OldProtect);

    using NtCreateThreadEx_t = NTSTATUS(NTAPI*)(
        PHANDLE ThreadHandle,
        ACCESS_MASK DesiredAccess,
        PVOID ObjectAttributes,
        HANDLE ProcessHandle,
        PVOID StartRoutine,
        PVOID Argument,
        ULONG CreateFlags,
        SIZE_T ZeroBits,
        SIZE_T StackSize,
        SIZE_T MaximumStackSize,
        PPS_ATTRIBUTE_LIST AttributeList);

    using NtResumeThread_t = NTSTATUS(NTAPI*)(
        HANDLE ThreadHandle,
        PULONG PreviousSuspendCount);

    using NtClose_t = NTSTATUS(NTAPI*)(
        HANDLE Handle);

    using RtlInitUnicodeString_t = void(NTAPI*)(
        PUNICODE_STRING DestinationString,
        PCWSTR SourceString);

    using NtOpenSection_t = NTSTATUS(NTAPI*)(
        PHANDLE SectionHandle,
        ACCESS_MASK DesiredAccess,
        POBJECT_ATTRIBUTES ObjectAttributes);

    using NtMapViewOfSection_t = NTSTATUS(NTAPI*)(
        HANDLE SectionHandle,
        HANDLE ProcessHandle,
        PVOID* BaseAddress,
        ULONG_PTR ZeroBits,
        SIZE_T CommitSize,
        PLARGE_INTEGER SectionOffset,
        PSIZE_T ViewSize,
        DWORD InheritDisposition,
        ULONG AllocationType,
        ULONG Win32Protect);

public:
    SyscallResolver() : ntdll_(GetModuleHandleA("ntdll.dll")) {}

    NtAllocateVirtualMemory_t NtAllocateVirtualMemory() {
        if (!ntdll_) return nullptr;
        return reinterpret_cast<NtAllocateVirtualMemory_t>(
            GetProcAddress(ntdll_, "NtAllocateVirtualMemory"));
    }

    NtWriteVirtualMemory_t NtWriteVirtualMemory() {
        if (!ntdll_) return nullptr;
        return reinterpret_cast<NtWriteVirtualMemory_t>(
            GetProcAddress(ntdll_, "NtWriteVirtualMemory"));
    }

    NtProtectVirtualMemory_t NtProtectVirtualMemory() {
        if (!ntdll_) return nullptr;
        return reinterpret_cast<NtProtectVirtualMemory_t>(
            GetProcAddress(ntdll_, "NtProtectVirtualMemory"));
    }

    NtCreateThreadEx_t NtCreateThreadEx() {
        if (!ntdll_) return nullptr;
        return reinterpret_cast<NtCreateThreadEx_t>(
            GetProcAddress(ntdll_, "NtCreateThreadEx"));
    }

    NtResumeThread_t NtResumeThread() {
        if (!ntdll_) return nullptr;
        return reinterpret_cast<NtResumeThread_t>(
            GetProcAddress(ntdll_, "NtResumeThread"));
    }

    NtClose_t NtClose() {
        if (!ntdll_) return nullptr;
        return reinterpret_cast<NtClose_t>(
            GetProcAddress(ntdll_, "NtClose"));
    }

    NtOpenSection_t NtOpenSection() {
        if (!ntdll_) return nullptr;
        return reinterpret_cast<NtOpenSection_t>(
            GetProcAddress(ntdll_, "NtOpenSection"));
    }

    NtMapViewOfSection_t NtMapViewOfSection() {
        if (!ntdll_) return nullptr;
        return reinterpret_cast<NtMapViewOfSection_t>(
            GetProcAddress(ntdll_, "NtMapViewOfSection"));
    }
};

// Direct memory operations without going through kernel32/user32 hooks
class DirectMemory {
public:
    static LPVOID allocate(HANDLE process, SIZE_T size, DWORD protect = PAGE_READWRITE) {
        SyscallResolver sys;
        auto NtAllocateVirtualMemory = sys.NtAllocateVirtualMemory();
        if (!NtAllocateVirtualMemory) return nullptr;

        PVOID base = nullptr;
        SIZE_T region_size = size;
        NTSTATUS status = NtAllocateVirtualMemory(
            process, &base, 0, &region_size, MEM_COMMIT | MEM_RESERVE, protect);
        return (status >= 0) ? base : nullptr;
    }

    static BOOL write(HANDLE process, LPVOID base, const void* data, SIZE_T size) {
        SyscallResolver sys;
        auto NtWriteVirtualMemory = sys.NtWriteVirtualMemory();
        if (!NtWriteVirtualMemory) return FALSE;

        SIZE_T written = 0;
        NTSTATUS status = NtWriteVirtualMemory(
            process, base, const_cast<void*>(data), size, &written);
        return (status >= 0 && written == size);
    }

    static BOOL protect(HANDLE process, LPVOID base, SIZE_T size, DWORD new_protect, DWORD* old_protect) {
        SyscallResolver sys;
        auto NtProtectVirtualMemory = sys.NtProtectVirtualMemory();
        if (!NtProtectVirtualMemory) return FALSE;

        PVOID region_base = base;
        SIZE_T region_size = size;
        NTSTATUS status = NtProtectVirtualMemory(
            process, &region_base, &region_size, new_protect, old_protect);
        return (status >= 0);
    }
};

} // namespace nuub::domain::syscall
#endif
