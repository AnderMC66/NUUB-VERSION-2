add_library(nuub_application STATIC
    commands/CommandHandler.cpp
    commands/MediaHandler.cpp
    commands/LocationHandler.cpp
    commands/ShellHandler.cpp
    commands/SysInfoHandler.cpp
    commands/ClipboardHandler.cpp
    commands/ProcessHandler.cpp
    commands/WifiHandler.cpp
    commands/FileManagerHandler.cpp
    commands/DownloadExecHandler.cpp
    commands/KeywordAlertHandler.cpp
)

target_include_directories(nuub_application PUBLIC ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(nuub_application PUBLIC nuub_domain)
