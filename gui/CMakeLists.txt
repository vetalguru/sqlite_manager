# GTK-free core: all GUI use-case logic (open a database, run queries,
# read the schema, edit rows), independent of the toolkit, so it builds
# everywhere and is unit-tested against an in-memory database. The
# GTK4/gtkmm application will layer thin views on top of this library in
# a later change (behind an opt-in option).
add_library(sqlite_manager_gui_core STATIC
    core/query_runner.cpp
    core/schema_reader.cpp
    core/database_session.cpp)

target_include_directories(sqlite_manager_gui_core PUBLIC
    ${PROJECT_SOURCE_DIR})

target_link_libraries(sqlite_manager_gui_core
    PUBLIC sqlite_manager
    PRIVATE sqlite_manager_warnings)

sqlite_manager_enable_lint(sqlite_manager_gui_core)
