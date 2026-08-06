include(FetchContent)

FetchContent_Declare(
    googletest
    URL https://github.com/google/googletest/archive/refs/tags/v1.17.0.tar.gz
    URL_HASH SHA256=65fab701d9829d38cb77c14acdc431d2108bfdbf8979e40eb8ae567edf10b27c
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE)

# Prevent GTest from overriding compiler/linker options on Windows
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)

FetchContent_MakeAvailable(googletest)

add_executable(sqlite_manager_tests
    version_test.cpp
    result_test.cpp
    connection_test.cpp
    statement_test.cpp
    transaction_test.cpp
    arg_parser_test.cpp
    result_view_test.cpp
    cli_application_test.cpp)

target_link_libraries(sqlite_manager_tests
    PRIVATE
        sqlite_manager
        sqlite_manager_cli_lib
        sqlite_manager_warnings
        GTest::gtest_main)

include(GoogleTest)
gtest_discover_tests(sqlite_manager_tests)