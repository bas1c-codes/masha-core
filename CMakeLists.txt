cmake_minimum_required(VERSION 3.14)
project(masha)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(OpenSSL REQUIRED)
find_package(CLI11 CONFIG REQUIRED)
find_package(unofficial-libyara CONFIG REQUIRED)
add_library(core
    core/hash.cpp
    core/scan.cpp
    core/load.cpp
    core/quarantine.cpp
    core/encrypt.cpp
    core/yara.cpp
)

target_link_libraries(core PRIVATE OpenSSL::SSL OpenSSL::Crypto Crypt32 unofficial::libyara::libyara)
add_executable(cli
    cli/main.cpp
)

target_link_libraries(cli PRIVATE core CLI11::CLI11)

add_executable(service
    service/service.cpp  # Your service source file(s)
    service/usn.cpp
    service/log.cpp
)

target_link_libraries(service PRIVATE core ws2_32 advapi32 Crypt32)

# Remove WIN32_EXECUTABLE to avoid WinMain unresolved error
# If you want to keep it, your code must define WinMain entry point
