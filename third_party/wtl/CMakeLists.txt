# Windows Template Library (WTL): a header-only C++ UI framework layered on
# ATL. Consumed only by the opt-in Windows GUI (gui/wtl) - never by the
# library, the CLI, or the GTK GUI - so this INTERFACE target is harmless to
# define on every platform. ATL itself is provided by the MSVC toolchain and
# is not vendored here.
add_library(wtl INTERFACE)
target_include_directories(wtl SYSTEM INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/Include)
