# Computes a component's version from a mix of CMake and git.
#
#   MAJOR.MINOR  come from the caller - the single source of truth,
#                bumped by hand (project(VERSION ...) or a set()).
#   PATCH        = number of git commits touching the component's paths
#                since the most recent matching release tag. Editing the
#                component and committing bumps the patch automatically;
#                tagging a new release resets it.
#
# Without git (e.g. a source tarball) PATCH falls back to 0.
#
# Usage:
#   include(GitVersion)
#   sqlite_manager_git_version(
#       OUT       LIB              # variable name prefix for results
#       MAJOR     0
#       MINOR     1
#       TAG_MATCH "v*"             # glob of this component's release tags
#       PATHS     src include)     # commits here bump the patch
#   # -> LIB_PATCH   = 4
#   # -> LIB_VERSION = "0.1.4"

find_package(Git QUIET)

function(sqlite_manager_git_version)
    cmake_parse_arguments(ARG "" "OUT;MAJOR;MINOR;TAG_MATCH" "PATHS" ${ARGN})

    set(patch 0)

    if(Git_FOUND AND EXISTS "${PROJECT_SOURCE_DIR}/.git")
        # Most recent matching release tag, if any.
        execute_process(
            COMMAND "${GIT_EXECUTABLE}" describe --tags --abbrev=0
                    --match "${ARG_TAG_MATCH}"
            WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}"
            OUTPUT_VARIABLE last_tag
            OUTPUT_STRIP_TRAILING_WHITESPACE
            RESULT_VARIABLE tag_found
            ERROR_QUIET)

        if(tag_found EQUAL 0 AND last_tag)
            set(range "${last_tag}..HEAD")
        else()
            set(range "HEAD")   # no release yet: count the whole history
        endif()

        # Patch = commits since that point touching the component paths.
        execute_process(
            COMMAND "${GIT_EXECUTABLE}" rev-list --count ${range} -- ${ARG_PATHS}
            WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}"
            OUTPUT_VARIABLE patch
            OUTPUT_STRIP_TRAILING_WHITESPACE
            ERROR_QUIET)
        if(NOT patch MATCHES "^[0-9]+$")
            set(patch 0)
        endif()

        # Re-run configuration when the checked-out commit or index moves,
        # so the version is recomputed on the next build after a commit.
        foreach(dep HEAD index)
            if(EXISTS "${PROJECT_SOURCE_DIR}/.git/${dep}")
                set_property(DIRECTORY "${PROJECT_SOURCE_DIR}" APPEND
                    PROPERTY CMAKE_CONFIGURE_DEPENDS
                        "${PROJECT_SOURCE_DIR}/.git/${dep}")
            endif()
        endforeach()
    endif()

    set(${ARG_OUT}_PATCH   "${patch}"                          PARENT_SCOPE)
    set(${ARG_OUT}_VERSION "${ARG_MAJOR}.${ARG_MINOR}.${patch}" PARENT_SCOPE)
endfunction()
