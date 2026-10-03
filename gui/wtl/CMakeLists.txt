# Toolkit-free presentation logic for the WTL views: it turns a QueryResult
# into list-view columns and rows. No Win32 dependency, so it builds and is
# unit-tested on every platform; the Windows GUI below links it too.
add_library(sqlite_manager_wtl_model STATIC result_model.cpp)
target_include_directories(sqlite_manager_wtl_model PUBLIC ${PROJECT_SOURCE_DIR})
target_link_libraries(sqlite_manager_wtl_model
    PUBLIC sqlite_manager
    PRIVATE sqlite_manager_warnings)
sqlite_manager_enable_lint(sqlite_manager_wtl_model)

# The WTL GUI application. Opt-in and Windows/MSVC only - WTL extends ATL,
# which ships with MSVC - so a plain build never requires it:
#   cmake --preset windows-wtl
# The executable and its views (which pull in <windows.h>/WTL) are added in
# a later change; this guard only validates the toolchain for now.
option(SQLITE_MANAGER_BUILD_WTL_GUI
    "Build the WTL GUI application (Windows/MSVC only)" OFF)

if(SQLITE_MANAGER_BUILD_WTL_GUI AND NOT (WIN32 AND MSVC))
    message(FATAL_ERROR
        "SQLITE_MANAGER_BUILD_WTL_GUI requires Windows with MSVC (WTL/ATL).")
endif()
