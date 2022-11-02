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
set(NOT_MSVC ON)
if (MSVC)
    set(NOT_MSVC OFF)
endif ()
option(FUTURES_ALWAYS_LINT "Run the linter before running unit tests" ${NOT_MSVC})

#######################################################
### How to build                                    ###
#######################################################
option(FUTURES_PEDANTIC_WARNINGS "Use pedantic warnings." ON)
option(FUTURES_WARNINGS_AS_ERRORS "Treat warnings as errors." ON)
option(FUTURES_SANITIZERS "Build with sanitizers." ${DEBUG_MODE})
option(FUTURES_THREAD_SANITIZER "Use thread sanitizer instead of other sanitizers." OFF)
option(FUTURES_CATCH2_REPORTER "Reporter Catch2 should use when invoked from ctest." console)
option(FUTURES_TESTS_SMALL_POOL "Run tests with a default thread pool of size 1." OFF)
option(FUTURES_TIME_TRACE "Enable clang time-trace." ON)

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

    if (GCC)
        # This whole project is for coverage
        if (FUTURES_BUILD_COVERAGE_REPORT)
            if (NOT (CMAKE_BUILD_TYPE STREQUAL "Debug"))
                message(WARNING "Code coverage results with an optimized (non-Debug) build may be misleading")
            endif ()

            set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} --coverage -fprofile-arcs -ftest-coverage")
            set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} --coverage -fprofile-arcs -ftest-coverage")
        endif ()
    endif ()

    if (CLANG)
        if (FUTURES_BUILD_COVERAGE_REPORT)
            set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fprofile-instr-generate -fcoverage-mapping")
            set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fprofile-instr-generate -fcoverage-mapping")
        endif ()

        # Time tracing
        if (FUTURES_TIME_TRACE AND CLANG AND CLANG_VERSION_MAJOR GREATER_EQUAL 9)
            add_compile_options(-ftime-trace)
        endif ()
    endif ()

    if (MSVC)
        # Allow exceptions in MSVC
        if (FUTURES_BUILD_WITH_EXCEPTIONS)
            add_compile_options(/EHsc)
        endif ()
        # Allow utf-8 in MSVC
        if (FUTURES_BUILD_WITH_UTF8 AND MSVC)
            add_compile_options(/utf-8)
        endif ()
    endif ()

    # Maybe add sanitizers to all targets
    if (NOT EMSCRIPTEN)
        if (FUTURES_THREAD_SANITIZER)
            add_sanitizer("thread")
        elseif (FUTURES_SANITIZERS)
            add_sanitizers()
        endif ()
    endif ()
endif ()
