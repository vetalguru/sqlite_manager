# SQLite amalgamation as a static library.
# sqlite3.c / sqlite3.h are vendored verbatim from sqlite.org.
add_library(sqlite3 STATIC sqlite3.c)

# Third-party code we do not modify: silence all warnings.
target_compile_options(sqlite3 PRIVATE -w)

target_include_directories(sqlite3 PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR})

# Recommended compile-time options (see https://www.sqlite.org/compile.html)
target_compile_definitions(sqlite3 PRIVATE
    SQLITE_DQS=0                  # disable double-quoted string literals misfeature
    SQLITE_THREADSAFE=1           # serialized mode (safe default)
    SQLITE_DEFAULT_MEMSTATUS=0    # skip memory usage tracking
    SQLITE_OMIT_DEPRECATED=1
    SQLITE_OMIT_LOAD_EXTENSION=1) # no dynamic extension loading

find_package(Threads REQUIRED)
target_link_libraries(sqlite3 PRIVATE Threads::Threads ${CMAKE_DL_LIBS})