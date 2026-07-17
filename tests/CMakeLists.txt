find_package(GTest CONFIG REQUIRED)

add_executable(nuub_tests
    domain/KeystrokeServiceTest.cpp
    domain/EncryptionServiceTest.cpp
    domain/AdminTest.cpp
    application/CommandHandlerTest.cpp
    application/MediaHandlerTest.cpp
    application/LocationHandlerTest.cpp
    application/ShellHandlerTest.cpp
)

target_include_directories(nuub_tests PRIVATE ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(nuub_tests PRIVATE
    GTest::gtest
    GTest::gtest_main
    nuub_domain
    nuub_application
)

include(GoogleTest)
gtest_discover_tests(nuub_tests)
