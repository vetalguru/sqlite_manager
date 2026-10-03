add_subdirectory(sqlite)

# Line editing for the interactive CLI (cross-platform).
add_subdirectory(isocline)

# Windows Template Library headers for the opt-in Windows GUI (gui/wtl).
# Header-only INTERFACE target, harmless on non-Windows builds.
add_subdirectory(wtl)