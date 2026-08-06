add_library(sqlite_manager_cli_lib STATIC
    arg_parser.cpp
    cli_application.cpp
    repl.cpp
    sql_executor.cpp)

target_include_directories(sqlite_manager_cli_lib PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(sqlite_manager_cli_lib
    PUBLIC sqlite_manager
    PRIVATE sqlite_manager_warnings)

add_executable(sqlite_manager_cli main.cpp)

set_target_properties(sqlite_manager_cli PROPERTIES OUTPUT_NAME sqlite-manager)

target_link_libraries(sqlite_manager_cli
    PRIVATE
        sqlite_manager_cli_lib
        sqlite_manager_warnings)