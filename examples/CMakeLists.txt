# Generate the demo database from demo.sql using the freshly built CLI,
# so examples/demo.db is ready to open in the GUI or the CLI. It is
# regenerated only when it is missing or when demo.sql changes; the .db
# itself is a build artifact and is git-ignored.
option(SQLITE_MANAGER_BUILD_EXAMPLES
    "Generate the demo database at build time" ON)

if(SQLITE_MANAGER_BUILD_EXAMPLES)
    set(demo_sql ${CMAKE_CURRENT_SOURCE_DIR}/demo.sql)
    set(demo_db_file ${CMAKE_CURRENT_SOURCE_DIR}/demo.db)

    # The CLI reads the script from stdin in --batch mode; a shell handles
    # the redirection (the build targets Linux/WSL).
    add_custom_command(
        OUTPUT ${demo_db_file}
        COMMAND ${CMAKE_COMMAND} -E rm -f ${demo_db_file}
        COMMAND /bin/sh -c
                "'$<TARGET_FILE:sqlite_manager_cli>' --batch '${demo_db_file}' < '${demo_sql}'"
        DEPENDS sqlite_manager_cli ${demo_sql}
        COMMENT "Generating examples/demo.db from demo.sql"
        VERBATIM)

    add_custom_target(demo-db ALL DEPENDS ${demo_db_file})
endif()
