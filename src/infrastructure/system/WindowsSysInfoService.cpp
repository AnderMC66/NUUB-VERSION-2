#include "infrastructure/system/WindowsSysInfoService.hpp"

#include <sstream>
#include <array>

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <iphlpapi.h>

#pragma comment(lib, "iphlpapi.lib")
#pragma comment(lib, "ws2_32.lib")

namespace nuub::infrastructure::system {

std::string WindowsSysInfoService::get_system_info() {
    std::ostringstream oss;

    // OS Version
    OSVERSIONINFOEXA osvi{};
    osvi.dwOSVersionInfoSize = sizeof(osvi);
    // Use RtlGetVersion for accurate version info
    using RtlGetVersion = LONG(WINAPI*)(PRTL_OSVERSIONINFOW);
    auto rtlGetVersion = reinterpret_cast<RtlGetVersion>(
        GetProcAddress(GetModuleHandleA("ntdll.dll"), "RtlGetVersion"));
    if (rtlGetVersion) {
        RTL_OSVERSIONINFOW rovi{};
        rovi.dwOSVersionInfoSize = sizeof(rovi);
        rtlGetVersion(&rovi);
        oss << "OS: Windows " << rovi.dwMajorVersion << "." << rovi.dwMinorVersion
            << " (Build " << rovi.dwBuildNumber << ")\n";
    }

    // Computer name
    char computer_name[MAX_COMPUTERNAME_LENGTH + 1]{};
    DWORD size = MAX_COMPUTERNAME_LENGTH + 1;
    GetComputerNameA(computer_name, &size);
    oss << "PC: " << computer_name << "\n";

    // Username
    char username[256]{};
    DWORD username_size = 256;
    GetUserNameA(username, &username_size);
    oss << "User: " << username << "\n";

    // CPU info
    SYSTEM_INFO sys_info;
    GetSystemInfo(&sys_info);
    oss << "CPU Cores: " << sys_info.dwNumberOfProcessors << "\n";

    // Memory
    MEMORYSTATUSEX mem_status{};
    mem_status.dwLength = sizeof(mem_status);
    if (GlobalMemoryStatusEx(&mem_status)) {
        oss << "RAM Total: " << (mem_status.ullTotalPhys / (1024 * 1024)) << " MB\n";
        oss << "RAM Used: " << ((mem_status.ullTotalPhys - mem_status.ullAvailPhys) / (1024 * 1024)) << " MB\n";
        oss << "RAM Free: " << (mem_status.ullAvailPhys / (1024 * 1024)) << " MB\n";
        oss << "Memory Usage: " << mem_status.dwMemoryLoad << "%\n";
    }

    // Disk space
    ULARGE_INTEGER free_bytes, total_bytes;
    if (GetDiskFreeSpaceExA("C:\\", &free_bytes, &total_bytes, nullptr)) {
        oss << "Disk C: Total: " << (total_bytes.QuadPart / (1024 * 1024 * 1024)) << " GB\n";
        oss << "Disk C: Free: " << (free_bytes.QuadPart / (1024 * 1024 * 1024)) << " GB\n";
    }

    // Uptime
    ULONGLONG uptime_ms = GetTickCount64();
    auto hours = uptime_ms / (1000 * 60 * 60);
    auto minutes = (uptime_ms / (1000 * 60)) % 60;
    oss << "Uptime: " << hours << "h " << minutes << "m\n";

    // IP addresses
    char hostname[256]{};
    gethostname(hostname, sizeof(hostname));
    oss << "Hostname: " << hostname << "\n";

    struct addrinfo hints{}, *result = nullptr;
    hints.ai_family = AF_INET;
    if (getaddrinfo(hostname, nullptr, &hints, &result) == 0 && result) {
        char ip[INET_ADDRSTRLEN]{};
        auto* addr = reinterpret_cast<sockaddr_in*>(result->ai_addr);
        inet_ntop(AF_INET, &addr->sin_addr, ip, sizeof(ip));
        oss << "Local IP: " << ip << "\n";
        freeaddrinfo(result);
    }

    return oss.str();
}

} // namespace nuub::infrastructure::system
