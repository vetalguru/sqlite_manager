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
#   cmake --build build/windows-wtl --target sqlite-manager-wtl
# Project warning flags and clang-tidy are deliberately not applied to the
# WTL views, as with the GTK GUI (ATL/WTL's template-heavy headers would
# drown out our own diagnostics).
option(SQLITE_MANAGER_BUILD_WTL_GUI
    "Build the WTL GUI application (Windows/MSVC only)" OFF)

if(SQLITE_MANAGER_BUILD_WTL_GUI)
    if(NOT (WIN32 AND MSVC))
        message(FATAL_ERROR
            "SQLITE_MANAGER_BUILD_WTL_GUI requires Windows with MSVC (WTL/ATL).")
    endif()

    enable_language(RC)  # compile the menu/resource script
    add_executable(sqlite-manager-wtl WIN32 main.cpp sqlite_manager.rc)
    target_include_directories(sqlite-manager-wtl PRIVATE ${PROJECT_SOURCE_DIR})
    target_compile_definitions(sqlite-manager-wtl PRIVATE
        UNICODE _UNICODE WIN32_LEAN_AND_MEAN NOMINMAX STRICT)
    target_link_libraries(sqlite-manager-wtl
        PRIVATE
            sqlite_manager_wtl_model
            sqlite_manager_gui_core
            wtl
            comctl32)
endif()
