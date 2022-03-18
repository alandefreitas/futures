#######################################################
### Stress Tests                                    ###
#######################################################
include(ProcessorCount)
ProcessorCount(N_PROCESSORS)

# Macro to create test targets and register with ctest
# First parameter: filename without extension
# Other parameters: libraries to link in this test (if empty, it links libcppm)
macro(add_futures_stress_test TEST_NAME)
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
            add_executable(stress_${TEST_NAME_VARIANT} ${TEST_NAME}.cpp)

            # Enable UTF-8 on windows
            target_msvc_compile_options(stress_${TEST_NAME_VARIANT} "/utf-8")

            # Enable /bigobj on windows
            target_msvc_compile_options(stress_${TEST_NAME_VARIANT} "/bigobj")

            # Register with ctest
            add_test(NAME stress_${TEST_NAME_VARIANT} COMMAND stress_${TEST_NAME_VARIANT} 10)

            # Link futures
            if (${TEST_NAME_VARIANT} STREQUAL ${TEST_NAME}_single_header)
                target_link_libraries(stress_${TEST_NAME_VARIANT} PUBLIC futures::single-header-futures)
            else()
                target_link_libraries(stress_${TEST_NAME_VARIANT} PUBLIC futures::futures-compiled)
            endif()

            # Include boost compile options
            if (${TEST_NAME_VARIANT} STREQUAL ${TEST_NAME}_boost)
                target_compile_definitions(stress_${TEST_NAME_VARIANT} INTERFACE FUTURES_PREFER_BOOST_DEPENDENCIES=1)
                target_compile_definitions(stress_${TEST_NAME_VARIANT} INTERFACE FUTURES_PREFER_STANDALONE_DEPENDENCIES=0)
            endif()

            # Define size of thread pool
            if (${TEST_NAME_VARIANT} STREQUAL ${TEST_NAME}_small_pool)
                # Purposefully set thread pool size to 1 in a target variant to ensure tests work for
                # all pool sizes. Some tests might lead to deadlocks that won't happen in large pools.
                target_compile_definitions(stress_${TEST_NAME_VARIANT} PUBLIC FUTURES_DEFAULT_THREAD_POOL_SIZE=1)
            endif ()

            if (${TEST_NAME_VARIANT} STREQUAL ${TEST_NAME}_single_header)
                target_compile_definitions(stress_${TEST_NAME_VARIANT} PUBLIC FUTURES_IS_SINGLE_HEADER=1)
            endif ()

            # Define test case prefix
            if (${TEST_NAME_VARIANT} STREQUAL ${TEST_NAME}_single_header)
                target_compile_definitions(stress_${TEST_NAME_VARIANT} PUBLIC TEST_CASE_PREFIX="Single header ")
            elseif (${TEST_NAME_VARIANT} STREQUAL ${TEST_NAME}_small_pool)
                target_compile_definitions(stress_${TEST_NAME_VARIANT} PUBLIC TEST_CASE_PREFIX="Small pool ")
            elseif (${TEST_NAME_VARIANT} STREQUAL ${TEST_NAME}_boost)
                target_compile_definitions(stress_${TEST_NAME_VARIANT} PUBLIC TEST_CASE_PREFIX="Boost ")
            else()
                target_compile_definitions(stress_${TEST_NAME_VARIANT} PUBLIC TEST_CASE_PREFIX="")
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
add_futures_stress_test(wait)
add_futures_stress_test(launch)