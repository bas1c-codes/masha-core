cmake_minimum_required(VERSION 3.14)
project(masha)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Add vcpkg packages
find_package(OpenSSL REQUIRED)
find_package(CLI11 CONFIG REQUIRED)
# Core library
add_library(core
    core/hash.cpp
    core/scan.cpp
    core/load.cpp
    core/quarantine.cpp
)

# Link OpenSSL and SQLite to core
target_link_libraries(core PRIVATE OpenSSL::SSL OpenSSL::Crypto)

# CLI executable
add_executable(cli
    cli/main.cpp
)

# Link core + CLI11 to executable
target_link_libraries(cli PRIVATE core CLI11::CLI11)
