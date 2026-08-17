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

# The GTK4/gtkmm application. Opt-in (needs libgtkmm-4.0-dev) so a plain
# build of the library and CLI never requires GTK:
#   cmake --preset linux-debug -DSQLITE_MANAGER_BUILD_GUI=ON
#   cmake --build build/debug --target sqlite-manager-gui
# The GUI links only the toolkit-free core; project warning flags and
# clang-tidy are deliberately not applied to it (gtkmm's template-heavy
# headers would drown out our own diagnostics).
option(SQLITE_MANAGER_BUILD_GUI "Build the GTK4/gtkmm GUI application" OFF)

if(SQLITE_MANAGER_BUILD_GUI)
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(GTKMM REQUIRED IMPORTED_TARGET gtkmm-4.0)

    add_executable(sqlite-manager-gui
        gtk/main.cpp
        gtk/application.cpp
        gtk/main_window.cpp)

    target_include_directories(sqlite-manager-gui PRIVATE ${PROJECT_SOURCE_DIR})

    target_link_libraries(sqlite-manager-gui
        PRIVATE
            sqlite_manager_gui_core
            PkgConfig::GTKMM)
endif()
