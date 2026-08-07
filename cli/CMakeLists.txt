# Everything except main.cpp lives in a static library so both the
# binary and the unit tests link the same code.
add_library(sqlite_manager_cli_lib STATIC
    arg_parser.cpp
    cli_application.cpp
    line_reader.cpp
    repl.cpp
    result_view.cpp
    sql_executor.cpp)

target_include_directories(sqlite_manager_cli_lib PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR})

# CLI version: its own MAJOR.MINOR (independent of the library); PATCH
# counts commits to cli/, src/, and include/ since the last "cli-v*" tag.
set(SQLITE_MANAGER_CLI_MAJOR 0)
set(SQLITE_MANAGER_CLI_MINOR 1)
sqlite_manager_git_version(
    OUT SQLITE_MANAGER_CLI
    MAJOR ${SQLITE_MANAGER_CLI_MAJOR}
    MINOR ${SQLITE_MANAGER_CLI_MINOR}
    TAG_MATCH "cli-v*"
    PATHS cli src include)

configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/cli_version.h.in
    ${PROJECT_BINARY_DIR}/generated/cli_version.h
    @ONLY)
target_include_directories(sqlite_manager_cli_lib PUBLIC
    ${PROJECT_BINARY_DIR}/generated)

target_link_libraries(sqlite_manager_cli_lib
    PUBLIC sqlite_manager
    PRIVATE sqlite_manager_warnings)

target_link_libraries(sqlite_manager_cli_lib PRIVATE isocline)

sqlite_manager_enable_lint(sqlite_manager_cli_lib)

add_executable(sqlite_manager_cli main.cpp)

set_target_properties(sqlite_manager_cli PROPERTIES OUTPUT_NAME sqlite-manager)

target_link_libraries(sqlite_manager_cli
    PRIVATE
        sqlite_manager_cli_lib
        sqlite_manager_warnings)

sqlite_manager_enable_lint(sqlite_manager_cli)