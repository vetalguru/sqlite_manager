# Everything except main.cpp lives in a static library so both the
# binary and the unit tests link the same code.
add_library(sqlite_manager_cli_lib STATIC
    arg_parser.cpp
    cli_application.cpp
    line_reader.cpp
    repl.cpp
    table_view.cpp
    sql_executor.cpp)

target_include_directories(sqlite_manager_cli_lib PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR})

# CLI version: its own MAJOR.MINOR (independent of the library); PATCH
# counts commits to cli/, man/, src/, and include/ since the last
# "cli-v*" tag (source, docs, and the statically linked library all
# affect the shipped package).
set(SQLITE_MANAGER_CLI_MAJOR 0)
set(SQLITE_MANAGER_CLI_MINOR 1)
sqlite_manager_git_version(
    OUT SQLITE_MANAGER_CLI
    MAJOR ${SQLITE_MANAGER_CLI_MAJOR}
    MINOR ${SQLITE_MANAGER_CLI_MINOR}
    TAG_MATCH "cli-v*"
    PATHS cli man src include)

configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/cli_version.h.in
    ${PROJECT_BINARY_DIR}/generated/cli_version.h
    @ONLY)
target_include_directories(sqlite_manager_cli_lib PUBLIC
    ${PROJECT_BINARY_DIR}/generated)

# Lift the CLI version to the parent scope so packaging can use it.
set(SQLITE_MANAGER_CLI_VERSION "${SQLITE_MANAGER_CLI_VERSION}" PARENT_SCOPE)

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

# Install the CLI binary (statically linked - no libraries to ship).
# This is what the .deb package wraps; see CPack setup in the root.
include(GNUInstallDirs)
install(TARGETS sqlite_manager_cli
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

# Man page: install to man1, gzip-compressed (Debian policy). Being a
# tracked installed file, it is removed by `apt remove` and by the
# `uninstall` target along with the binary - no extra removal handling.
set(man_page ${PROJECT_SOURCE_DIR}/man/sqlite-manager.1)
find_program(GZIP_EXE gzip)
if(GZIP_EXE)
    set(man_gz ${CMAKE_CURRENT_BINARY_DIR}/sqlite-manager.1.gz)
    add_custom_command(
        OUTPUT ${man_gz}
        COMMAND ${CMAKE_COMMAND} -E copy
                ${man_page} ${CMAKE_CURRENT_BINARY_DIR}/sqlite-manager.1
        COMMAND ${GZIP_EXE} -9 -n -f ${CMAKE_CURRENT_BINARY_DIR}/sqlite-manager.1
        DEPENDS ${man_page}
        COMMENT "Compressing man page")
    add_custom_target(sqlite_manager_man ALL DEPENDS ${man_gz})
    install(FILES ${man_gz} DESTINATION ${CMAKE_INSTALL_MANDIR}/man1)
else()
    # No gzip: install the raw page (man reads it fine, just uncompressed).
    install(FILES ${man_page} DESTINATION ${CMAKE_INSTALL_MANDIR}/man1)
endif()