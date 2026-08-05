add_library(sqlite_manager STATIC
    version.cpp
    error.cpp
    connection.cpp)

target_include_directories(sqlite_manager PUBLIC
    ${PROJECT_SOURCE_DIR}/include)

target_link_libraries(sqlite_manager
    PUBLIC sqlite3
    PRIVATE sqlite_manager_warnings)
