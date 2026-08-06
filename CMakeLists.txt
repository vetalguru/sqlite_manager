cmake_minimum_required(VERSION 3.22)

project(sqlite_manager
    VERSION 0.1.0
    DESCRIPTION "Lightweight C++17 wrapper over SQLite C API"
    LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Strict warnings for project code only (not for sqlite3.c)
add_library(sqlite_manager_warnings INTERFACE)
target_compile_options(sqlite_manager_warnings INTERFACE
    -Wall -Wextra -Wpedantic -Wconversion -Wshadow)

add_subdirectory(third_party)
add_subdirectory(src)
add_subdirectory(cli)

# Tests only when built as the top-level project
if(PROJECT_IS_TOP_LEVEL)
    enable_testing()
    add_subdirectory(tests)
endif()
