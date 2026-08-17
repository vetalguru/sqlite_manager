add_library(sqlite_manager STATIC
    version.cpp
    error.cpp
    connection.cpp
    statement.cpp
    transaction.cpp
    result_writer.cpp)

# Library version: MAJOR.MINOR from project(VERSION); PATCH counts
# commits to src/ and include/ since the last "lib-v*" tag.
sqlite_manager_git_version(
    OUT SQLITE_MANAGER_LIB
    MAJOR ${PROJECT_VERSION_MAJOR}
    MINOR ${PROJECT_VERSION_MINOR}
    TAG_MATCH "lib-v*"
    PATHS src include)

# Generate version.h from the template. Regenerated whenever the version
# inputs, the git commit/index, or the template change.
set(sqlite_manager_generated_dir ${CMAKE_CURRENT_BINARY_DIR}/generated)
configure_file(
    ${PROJECT_SOURCE_DIR}/include/sqlite_manager/version.h.in
    ${sqlite_manager_generated_dir}/sqlite_manager/version.h
    @ONLY)

target_include_directories(sqlite_manager PUBLIC
    ${PROJECT_SOURCE_DIR}/include
    ${sqlite_manager_generated_dir})

target_link_libraries(sqlite_manager
    PUBLIC sqlite3
    PRIVATE sqlite_manager_warnings)

sqlite_manager_enable_lint(sqlite_manager)
