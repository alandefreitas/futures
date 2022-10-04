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
        set(TEST_NAME_VARIANTS "")
        if (asio_FOUND OR Asio_FOUND)
            list(APPEND TEST_NAME_VARIANTS ${TEST_NAME}_asio)
        endif ()
        if (Boost_FOUND)
            list(APPEND TEST_NAME_VARIANTS ${TEST_NAME}_boost)
        endif ()
        if (N_PROCESSORS GREATER 1)
            list(APPEND TEST_NAME_VARIANTS ${TEST_NAME}_small_pool)
        endif ()

        # Create executables for each variant
        foreach (TEST_NAME_VARIANT ${TEST_NAME_VARIANTS})
            # Create executable for test
            add_executable(stress_${TEST_NAME_VARIANT} ${TEST_NAME}.cpp)

            # Link futures
            if (${TEST_NAME_VARIANT} STREQUAL ${TEST_NAME}_asio)
                target_link_libraries(stress_${TEST_NAME_VARIANT} PUBLIC futures_asio)
                target_compile_definitions(stress_${TEST_NAME_VARIANT} PUBLIC TEST_CASE_PREFIX="Asio ")
            elseif (${TEST_NAME_VARIANT} STREQUAL ${TEST_NAME}_boost)
                target_link_libraries(stress_${TEST_NAME_VARIANT} PUBLIC futures_boost)
                target_compile_definitions(stress_${TEST_NAME_VARIANT} PUBLIC TEST_CASE_PREFIX="Boost ")
            elseif (${TEST_NAME_VARIANT} STREQUAL ${TEST_NAME}_small_pool)
                target_link_libraries(stress_${TEST_NAME_VARIANT} PUBLIC futures)
                target_compile_definitions(stress_${TEST_NAME_VARIANT} PUBLIC TEST_CASE_PREFIX="Small pool ")
                target_compile_definitions(stress_${TEST_NAME_VARIANT} INTERFACE FUTURES_DEFAULT_THREAD_POOL_SIZE=1)
            endif ()

            # Register with ctest
            add_test(NAME stress_${TEST_NAME_VARIANT} COMMAND stress_${TEST_NAME_VARIANT} 10)
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