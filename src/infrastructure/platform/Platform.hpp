#pragma once

#if defined(_WIN32) || defined(_WIN64)
    #define NUUB_PLATFORM_WINDOWS
    #define NOMINMAX
    #include <Windows.h>
#elif defined(__linux__)
    #define NUUB_PLATFORM_LINUX
    #include <unistd.h>
    #include <signal.h>
#elif defined(__APPLE__)
    #define NUUB_PLATFORM_MACOS
    #include <unistd.h>
#else
    #error "Unsupported platform"
#endif
