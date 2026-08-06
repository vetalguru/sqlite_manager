add_executable(sqlite_manager_cli
    ${PROJECT_SOURCE_DIR}/cli/main.cpp
    ${PROJECT_SOURCE_DIR}/cli/arg_parser.cpp
    ${PROJECT_SOURCE_DIR}/cli/cli_application.cpp)

set_target_properties(sqlite_manager_cli PROPERTIES OUTPUT_NAME sqlite-manager)

target_link_libraries(sqlite_manager_cli
    PRIVATE
        sqlite_manager
        sqlite_manager_warnings)