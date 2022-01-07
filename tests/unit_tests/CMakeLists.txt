#######################################################
### Unit Tests                                      ###
#######################################################
# Include catch helper CMake scripts
include(${Catch2_DIR}/Catch.cmake)

# Create a common catch main for all tests
add_library(catch_main detail/catch_main.cpp)
target_link_libraries(catch_main PUBLIC Catch2::Catch2)
target_compile_features(catch_main PUBLIC cxx_std_17)

# Macro to create test targets and register with ctest
# First parameter: filename without extension
# Other parameters: libraries to link in this test (if empty, it links libcppm)
macro(add_futures_test TEST_NAME)
    # Identify libraries to link with, or link to futures
    set(EXTRA_MACRO_ARGS ${ARGN})
    list(LENGTH EXTRA_MACRO_ARGS NUM_EXTRA_ARGS)
    if (${NUM_EXTRA_ARGS} GREATER 0)
        set(LINK_LIBS ${ARGN})
    else ()
        set(LINK_LIBS futures::futures-compiled)
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
        foreach(TEST_NAME_VARIANT ${TEST_NAME} ${TEST_NAME}_small_pool)
            # Create executable for test
            add_executable(ut_${TEST_NAME_VARIANT} ${TEST_NAME}.cpp)

            # Link with catch-main
            target_link_libraries(ut_${TEST_NAME_VARIANT} PUBLIC ${LINK_LIBS} catch_main)

            # Enable UTF-8 on windows
            target_msvc_compile_options(ut_${TEST_NAME_VARIANT} "/utf-8")

            # Enable /bigobj on windows
            target_msvc_compile_options(ut_${TEST_NAME_VARIANT} "/bigobj")

            # Register with ctest
            catch_discover_tests(ut_${TEST_NAME_VARIANT})

            if (${TEST_NAME_VARIANT} STREQUAL ${TEST_NAME}_small_pool)
                # Purposefully set thread pool size to 1 in a target variant to ensure tests work for
                # all pool sizes. Some tests might lead to deadlocks that won't happen in large pools.
                target_compile_definitions(ut_${TEST_NAME_VARIANT} PUBLIC FUTURES_DEFAULT_THREAD_POOL_SIZE=1)
                target_compile_definitions(ut_${TEST_NAME_VARIANT} PUBLIC TEST_CASE_PREFIX="Small pool ")
            else()
                target_compile_definitions(ut_${TEST_NAME_VARIANT} PUBLIC TEST_CASE_PREFIX="")
            endif()
        endforeach()
    else ()
        # Library not found. Throw.
        message(FATAL_ERROR "${LINK_LIBS} does not exist")
    endif ()
endmacro()

# Main tests
add_futures_test(asio_executors)
add_futures_test(shared_state)
add_futures_test(futures_types)
add_futures_test(make_ready)
add_futures_test(async_overloads)
add_futures_test(continuations)
add_futures_test(conjunction)
add_futures_test(disjunction)
add_futures_test(cancel_futures)
add_futures_test(continuation_stop)
add_futures_test(exceptions)
add_futures_test(shared_futures)
add_futures_test(futures-algorithm)
