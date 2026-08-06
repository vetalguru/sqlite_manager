# isocline: cross-platform line editing (MIT, https://github.com/daanx/isocline)
# src/isocline.c is an amalgamation that #includes the other sources.
add_library(isocline STATIC src/isocline.c)

# Third-party code we do not modify: silence all warnings.
target_compile_options(isocline PRIVATE -w)

target_include_directories(isocline PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include)