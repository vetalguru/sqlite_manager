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

# Optional gcov/gcovr coverage instrumentation. It rides on the warnings
# interface, which is linked to exactly our targets (never third_party),
# so `--coverage` reaches all project code and the test executable without
# editing each subdirectory's CMakeLists. See the `coverage` target below.
option(SQLITE_MANAGER_COVERAGE "Instrument project code for coverage" OFF)
add_library(sqlite_manager_coverage INTERFACE)
if(SQLITE_MANAGER_COVERAGE)
    target_compile_options(sqlite_manager_coverage INTERFACE --coverage -O0 -g)
    target_link_options(sqlite_manager_coverage INTERFACE --coverage)
endif()
target_link_libraries(sqlite_manager_warnings INTERFACE sqlite_manager_coverage)

# Locate clang-tidy once, preferring the version CI pins (18) so a local
# run matches CI. Shared by the opt-in in-build linting and the `lint`
# target below.
find_program(CLANG_TIDY_EXE NAMES clang-tidy-18 clang-tidy)

# Opt-in in-build clang-tidy, wired onto project targets only (see
# .clang-tidy). Off by default so a plain build never needs clang-tidy.
option(SQLITE_MANAGER_ENABLE_CLANG_TIDY
    "Run clang-tidy on project sources during the build" OFF)

if(SQLITE_MANAGER_ENABLE_CLANG_TIDY AND NOT CLANG_TIDY_EXE)
    message(WARNING
        "SQLITE_MANAGER_ENABLE_CLANG_TIDY is ON but clang-tidy was not found")
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
add_subdirectory(gui)

# Tests and examples only when built as the top-level project
if(PROJECT_IS_TOP_LEVEL)
    enable_testing()
    add_subdirectory(tests)
    add_subdirectory(examples)
endif()

# `cmake --build <build-dir> --target lint` runs clang-tidy over the
# project sources exactly as the Lint CI workflow does - findings are
# errors. Needs a compile database (the linux-debug preset exports one).
# Run it before pushing to catch what CI's clang-tidy would flag.
if(PROJECT_IS_TOP_LEVEL AND CLANG_TIDY_EXE)
    file(GLOB lint_sources CONFIGURE_DEPENDS
        ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/cli/*.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/gui/core/*.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/*.cpp)
    add_custom_target(lint
        COMMAND ${CLANG_TIDY_EXE} -p ${CMAKE_BINARY_DIR}
                --warnings-as-errors=* ${lint_sources}
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        COMMENT "Running clang-tidy (findings are errors)"
        VERBATIM)
endif()

# `cmake --build <build-dir> --target coverage` runs the tests and reports
# line coverage for project code (terminal summary + HTML + XML). Needs a
# coverage-instrumented build (the linux-coverage preset, i.e.
# SQLITE_MANAGER_COVERAGE=ON) and gcovr.
if(PROJECT_IS_TOP_LEVEL AND SQLITE_MANAGER_COVERAGE)
    find_program(GCOVR_EXE gcovr)
    if(GCOVR_EXE)
        add_custom_target(coverage
            COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure
            COMMAND ${CMAKE_COMMAND} -E make_directory
                    ${CMAKE_BINARY_DIR}/coverage
            COMMAND ${GCOVR_EXE} --root ${CMAKE_CURRENT_SOURCE_DIR}
                    --filter ${CMAKE_CURRENT_SOURCE_DIR}/src
                    --filter ${CMAKE_CURRENT_SOURCE_DIR}/cli
                    --filter ${CMAKE_CURRENT_SOURCE_DIR}/gui/core
                    --filter ${CMAKE_CURRENT_SOURCE_DIR}/include
                    --print-summary
                    --html-details ${CMAKE_BINARY_DIR}/coverage/index.html
                    --xml ${CMAKE_BINARY_DIR}/coverage.xml
            WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
            COMMENT "Running tests and generating a coverage report"
            VERBATIM)
    else()
        message(WARNING
            "SQLITE_MANAGER_COVERAGE is ON but gcovr was not found; "
            "the coverage target is unavailable")
    endif()
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
