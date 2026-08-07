cmake_minimum_required(VERSION 3.22)

project(sqlite_manager
    VERSION 0.1.0
    DESCRIPTION "Lightweight C++17 wrapper over SQLite C API"
    LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Per-component versions: MAJOR.MINOR from CMake, PATCH auto from git.
list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)
include(GitVersion)

# Strict warnings for project code only (not for sqlite3.c)
add_library(sqlite_manager_warnings INTERFACE)
target_compile_options(sqlite_manager_warnings INTERFACE
    -Wall -Wextra -Wpedantic -Wconversion -Wshadow)

# Opt-in clang-tidy, wired onto project targets only (see .clang-tidy).
# Off by default so a plain build never requires clang-tidy to be present.
option(SQLITE_MANAGER_ENABLE_CLANG_TIDY
    "Run clang-tidy on project sources during the build" OFF)

if(SQLITE_MANAGER_ENABLE_CLANG_TIDY)
    find_program(CLANG_TIDY_EXE NAMES clang-tidy)
    if(CLANG_TIDY_EXE)
        message(STATUS "clang-tidy enabled: ${CLANG_TIDY_EXE}")
    else()
        message(WARNING
            "SQLITE_MANAGER_ENABLE_CLANG_TIDY is ON but clang-tidy was not found")
    endif()
endif()

# Applies clang-tidy to a target when enabled and available; a no-op
# otherwise. Keeps third_party and GoogleTest targets untouched.
function(sqlite_manager_enable_lint target)
    if(SQLITE_MANAGER_ENABLE_CLANG_TIDY AND CLANG_TIDY_EXE)
        set_target_properties(${target} PROPERTIES
            CXX_CLANG_TIDY "${CLANG_TIDY_EXE}")
    endif()
endfunction()

add_subdirectory(third_party)
add_subdirectory(src)
add_subdirectory(cli)

# Tests only when built as the top-level project
if(PROJECT_IS_TOP_LEVEL)
    enable_testing()
    add_subdirectory(tests)
endif()

# Packaging: build a .deb of the CLI with, from the build directory:
#   cpack -G DEB
# The package wraps the statically linked sqlite-manager binary; its
# version follows the CLI's (MAJOR.MINOR from CMake, PATCH from git).
if(PROJECT_IS_TOP_LEVEL)
    set(CPACK_PACKAGE_NAME "sqlite-manager")
    set(CPACK_PACKAGE_VERSION "${SQLITE_MANAGER_CLI_VERSION}")
    set(CPACK_PACKAGE_DESCRIPTION_SUMMARY
        "Command-line shell for SQLite databases")
    set(CPACK_PACKAGE_CONTACT "VS <vetalguru@gmail.com>")
    set(CPACK_PACKAGING_INSTALL_PREFIX "/usr")

    set(CPACK_GENERATOR "DEB")
    set(CPACK_DEBIAN_PACKAGE_SECTION "database")
    set(CPACK_DEBIAN_PACKAGE_PRIORITY "optional")
    # Auto-detect runtime deps (libc, libstdc++, ...) via dpkg-shlibdeps.
    set(CPACK_DEBIAN_PACKAGE_SHLIBDEPS ON)
    # Standard "<name>_<version>_<arch>.deb" file name.
    set(CPACK_DEBIAN_FILE_NAME DEB-DEFAULT)

    include(CPack)
endif()

# Uninstall for the direct `cmake --install` path (deb installs use
# `apt remove sqlite-manager`). Removes exactly what was installed, per
# install_manifest.txt.
if(PROJECT_IS_TOP_LEVEL AND NOT TARGET uninstall)
    configure_file(
        ${CMAKE_CURRENT_SOURCE_DIR}/cmake/cmake_uninstall.cmake.in
        ${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake
        @ONLY)
    add_custom_target(uninstall
        COMMAND ${CMAKE_COMMAND} -P ${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake)
endif()
