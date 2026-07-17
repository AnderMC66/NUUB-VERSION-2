add_library(nuub_domain STATIC
    services/KeystrokeService.cpp
    services/EncryptionService.cpp
    services/ReportingService.cpp
)

target_include_directories(nuub_domain PUBLIC ${CMAKE_SOURCE_DIR}/src)

find_package(OpenSSL REQUIRED)
find_package(unofficial-argon2 CONFIG REQUIRED)
find_package(spdlog CONFIG REQUIRED)
find_package(ZLIB REQUIRED)
target_link_libraries(nuub_domain PUBLIC OpenSSL::Crypto unofficial::argon2::libargon2 spdlog::spdlog ZLIB::ZLIB)
