#######################################################
### Developer mode                                  ###
#######################################################
# Developer mode enables targets and code paths in the CMake scripts that are
# only relevant for the developer(s) of futures.
# Targets necessary to build the project must be provided unconditionally, so
# consumers can trivially build and package the project
if (MASTER_PROJECT)
    option(FUTURES_DEVELOPER_MODE "Enable developer mode" OFF)
    option(BUILD_SHARED_LIBS "Build shared libs." OFF)
endif ()

if (NOT FUTURES_DEVELOPER_MODE)
    return()
endif ()

#######################################################
### What to build                                   ###
#######################################################
# C++ targets
option(FUTURES_BUILD_TESTS "Build tests" ON)
option(FUTURES_BUILD_SINGLE_TARGET_TESTS "Build tests" OFF)
option(FUTURES_BUILD_EXAMPLES "Build examples" ON)

# Custom targets
option(FUTURES_BUILD_DOCS "Build documentation" OFF)
option(FUTURES_BUILD_COVERAGE_REPORT "Enable coverage support" OFF)
option(FUTURES_BUILD_LINT "Enable linting" OFF)
option(FUTURES_BUILD_LINTER "Build C++ project linter" ON)
option(FUTURES_ALWAYS_LINT "Run the linter before running unit tests" ON)


#######################################################
### How to build                                    ###
#######################################################
option(FUTURES_PEDANTIC_WARNINGS "Use pedantic warnings." ON)
option(FUTURES_WARNINGS_AS_ERRORS "Treat warnings as errors." ON)
option(FUTURES_SANITIZERS "Build with sanitizers." ${DEBUG_MODE})
option(FUTURES_CATCH2_REPORTER "Reporter Catch2 should use when invoked from ctest." console)
option(FUTURES_TESTS_SMALL_POOL "Run tests with a default thread pool of size 1." OFF)

#######################################################
### How to build                                    ###
#######################################################
option(FUTURES_BUILD_WITH_UTF8 "Accept utf-8 in MSVC by default." ON)

#######################################################
### Apply global developer options                  ###
#######################################################
# In development, we can set some options for all targets
if (MASTER_PROJECT)
    message("Setting global options")

    # This whole project is for coverage
    if (FUTURES_BUILD_COVERAGE_REPORT)
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} --coverage")
        set(CMAKE_C_FLAGS "${DCMAKE_C_FLAGS} --coverage")
    endif ()

    # Maybe add sanitizers to all targets
    if (FUTURES_SANITIZERS AND NOT EMSCRIPTEN)
        add_sanitizers()
    endif ()

    # Allow exceptions in MSVC
    if (MSVC AND FUTURES_BUILD_WITH_EXCEPTIONS)
        add_compile_options(/EHsc)
    endif ()

    # Allow utf-8 in MSVC
    if (FUTURES_BUILD_WITH_UTF8 AND MSVC)
        set(CMAKE_CXX_FLAGS "/utf-8")
    endif ()
endif ()