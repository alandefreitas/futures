#######################################################
### Dependencies                                    ###
#######################################################
find_package(Catch2 2.13.8 CONFIG)
if (Catch2_FOUND)
    include(${Catch2_DIR}/Catch.cmake)
else ()
    FetchContent_Declare(Catch2 URL https://github.com/catchorg/Catch2/archive/refs/tags/v2.13.8.zip)
    FetchContent_GetProperties(Catch2)
    if (NOT Catch2_POPULATED)
        FetchContent_Populate(Catch2)
        set(CMAKE_POLICY_DEFAULT_CMP0077 NEW)
        set(CATCH_USE_VALGRIND OFF) # "Perform SelfTests with Valgrind"
        set(CATCH_BUILD_EXAMPLES OFF) # "Build documentation examples"
        set(CATCH_BUILD_EXTRA_TESTS OFF) # "Build extra tests"
        set(CATCH_BUILD_STATIC_LIBRARY OFF) # "Builds static library from the main implementation. EXPERIMENTAL"
        set(CATCH_ENABLE_COVERAGE OFF) # "Generate coverage for codecov.io"
        set(CATCH_ENABLE_WERROR OFF) # "Enable all warnings as errors"
        set(CATCH_INSTALL_DOCS OFF) # "Install documentation alongside library"
        set(CATCH_INSTALL_HELPERS ON) # "Install contrib alongside library"
        add_subdirectory(${catch2_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR}/catch2)
        include(${catch2_SOURCE_DIR}/contrib/Catch.cmake)
    endif ()
endif ()

# Create a common catch main for all tests
add_library(catch_main detail/catch_main.cpp)
target_link_libraries(catch_main PUBLIC Catch2::Catch2)
target_compile_features(catch_main PUBLIC cxx_std_17)


#######################################################
### Unit Tests                                      ###
#######################################################
include(ProcessorCount)
ProcessorCount(N_PROCESSORS)

# Macro to create test targets and register with ctest
# First parameter: filename without extension
# Other parameters: libraries to link in this test (if empty, it links libcppm)
macro(add_futures_test TEST_NAME)
    # Identify libraries to link with, or link to futures
    set(EXTRA_MACRO_ARGS ${ARGN})
    list(LENGTH EXTRA_MACRO_ARGS NUM_EXTRA_ARGS)
    if (${NUM_EXTRA_ARGS} GREATER 0)
        set(LINK_LIBS ${ARGN})
    endif ()

    # Check if these libraries really exist
    set(LINK_LIBS_EXIST TRUE)
    foreach (LINK_LIB ${LINK_LIBS})
        if (NOT TARGET ${LINK_LIB})
            set(LINK_LIBS_EXIST FALSE)
            break()
        endif ()
    endforeach ()

    if (LINK_LIBS_EXIST)
        # Define test target variants
        set(TEST_NAME_VARIANTS ${TEST_NAME})
        if(N_PROCESSORS GREATER 1)
            list(APPEND TEST_NAME_VARIANTS ${TEST_NAME}_small_pool)
        endif()
        if (Boost_FOUND)
            list(APPEND TEST_NAME_VARIANTS ${TEST_NAME}_boost)
        endif()
        if (FUTURES_BUILD_SINGLE_HEADER)
            list(APPEND TEST_NAME_VARIANTS ${TEST_NAME}_single_header)
        endif()

        # Create executables for each variant
        foreach (TEST_NAME_VARIANT ${TEST_NAME_VARIANTS})
            # Create executable for test
            add_executable(ut_${TEST_NAME_VARIANT} ${TEST_NAME}.cpp)

            # Link with catch-main
            target_link_libraries(ut_${TEST_NAME_VARIANT} PUBLIC ${LINK_LIBS} catch_main)

            # Enable UTF-8 on windows
            target_msvc_compile_options(ut_${TEST_NAME_VARIANT} "/utf-8")

            # Enable /bigobj on windows
            target_msvc_compile_options(ut_${TEST_NAME_VARIANT} "/bigobj")

            # Register with ctest
            if (FUTURES_CATCH2_REPORTER STREQUAL "console")
                catch_discover_tests(ut_${TEST_NAME_VARIANT})
            else ()
                if (NOT EXISTS ${CMAKE_CURRENT_BINARY_DIR}/reports)
                    file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/reports)
                endif()
                catch_discover_tests(
                        ut_${TEST_NAME_VARIANT}
                        REPORTER ${FUTURES_CATCH2_REPORTER}
                        OUTPUT_DIR ${CMAKE_CURRENT_BINARY_DIR}/reports
                        OUTPUT_SUFFIX ".${FUTURES_CATCH2_REPORTER}"
                )
            endif()

            # Link futures
            if (${TEST_NAME_VARIANT} STREQUAL ${TEST_NAME}_single_header)
                target_link_libraries(ut_${TEST_NAME_VARIANT} PUBLIC futures::single-header-futures)
            else()
                target_link_libraries(ut_${TEST_NAME_VARIANT} PUBLIC futures::futures-compiled)
            endif()

            # Include boost compile options
            if (${TEST_NAME_VARIANT} STREQUAL ${TEST_NAME}_boost)
                target_compile_definitions(ut_${TEST_NAME_VARIANT} INTERFACE FUTURES_PREFER_BOOST_DEPENDENCIES=1)
                target_compile_definitions(ut_${TEST_NAME_VARIANT} INTERFACE FUTURES_PREFER_STANDALONE_DEPENDENCIES=0)
            endif()

            # Define size of thread pool
            if (${TEST_NAME_VARIANT} STREQUAL ${TEST_NAME}_small_pool)
                # Purposefully set thread pool size to 1 in a target variant to ensure tests work for
                # all pool sizes. Some tests might lead to deadlocks that won't happen in large pools.
                target_compile_definitions(ut_${TEST_NAME_VARIANT} PUBLIC FUTURES_DEFAULT_THREAD_POOL_SIZE=1)
            endif ()

            if (${TEST_NAME_VARIANT} STREQUAL ${TEST_NAME}_single_header)
                target_compile_definitions(ut_${TEST_NAME_VARIANT} PUBLIC FUTURES_IS_SINGLE_HEADER=1)
            endif ()

            # Define test case prefix
            if (${TEST_NAME_VARIANT} STREQUAL ${TEST_NAME}_single_header)
                target_compile_definitions(ut_${TEST_NAME_VARIANT} PUBLIC TEST_CASE_PREFIX="Single header ")
            elseif (${TEST_NAME_VARIANT} STREQUAL ${TEST_NAME}_small_pool)
                target_compile_definitions(ut_${TEST_NAME_VARIANT} PUBLIC TEST_CASE_PREFIX="Small pool ")
            elseif (${TEST_NAME_VARIANT} STREQUAL ${TEST_NAME}_boost)
                target_compile_definitions(ut_${TEST_NAME_VARIANT} PUBLIC TEST_CASE_PREFIX="Boost ")
            else()
                target_compile_definitions(ut_${TEST_NAME_VARIANT} PUBLIC TEST_CASE_PREFIX="")
            endif()
        endforeach ()
    else ()
        # Library not found. Throw.
        message(FATAL_ERROR "${LINK_LIBS} does not exist")
    endif ()
endmacro()

######################################################
# Separate CMake tests                               #
######################################################
# Detail
add_futures_test(asio_executors)
add_futures_test(queue)
add_futures_test(operation_state)

# Futures
add_futures_test(futures_types)
add_futures_test(make_ready)
add_futures_test(exceptions)
add_futures_test(launch)
add_futures_test(wait)

# Extensions
add_futures_test(shared_futures)
add_futures_test(cancel_futures)
add_futures_test(continuation_stop)

# Adaptors
add_futures_test(continuations)
add_futures_test(conjunction)
add_futures_test(disjunction)

# Algorithms
add_futures_test(futures-algorithm)

#####################################################
# All tests as a single target                      #
#####################################################
if (FUTURES_BUILD_SINGLE_TARGET_TESTS)
    file(GLOB_RECURSE FUTURES_TESTS_SOURCE CONFIGURE_DEPENDS ./*.cpp)
    add_executable(ut_futures_all detail/catch_main.cpp ${FUTURES_TESTS_SOURCE})
    target_link_libraries(ut_futures_all PUBLIC futures::futures-compiled Catch2::Catch2)
    target_compile_features(ut_futures_all PUBLIC cxx_std_17)
    target_compile_definitions(ut_futures_all PUBLIC TEST_CASE_PREFIX="")
endif()