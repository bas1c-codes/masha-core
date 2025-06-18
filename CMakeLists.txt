cmake_minimum_required(VERSION 3.14)
project(masha)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(OpenSSL REQUIRED)
find_package(CLI11 CONFIG REQUIRED)
find_package(unofficial-libyara CONFIG REQUIRED)

# === Core static library ===
add_library(core
    core/hash.cpp
    core/scan.cpp
    core/load.cpp
    core/quarantine.cpp
    core/encrypt.cpp
    core/yara.cpp
)

target_link_libraries(core
    PRIVATE
    OpenSSL::SSL
    OpenSSL::Crypto
    Crypt32
    unofficial::libyara::libyara
)

# === CLI executable ===
add_executable(cli
    cli/main.cpp
)

target_link_libraries(cli
    PRIVATE
    core
    CLI11::CLI11
)


