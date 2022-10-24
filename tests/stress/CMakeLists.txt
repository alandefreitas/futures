#######################################################
### Stress Tests                                    ###
#######################################################
include(ProcessorCount)
ProcessorCount(N_PROCESSORS)

# Macro to create test targets and register with ctest
# First parameter: filename without extension
# Other parameters: libraries to link in this test (if empty, it links libcppm)
macro(add_futures_stress_test TEST_FILE)
    # Create a name for the test
    string(REPLACE "/" "_" CMAKE_TEST_NAME ${TEST_FILE})
    string(REPLACE ".cpp" "" CMAKE_TEST_NAME ${CMAKE_TEST_NAME})
    set(CMAKE_TEST_NAME stress_${CMAKE_TEST_NAME})

    # Create executable for test
    add_executable(${CMAKE_TEST_NAME} ${TEST_FILE})

    # Link futures
    target_link_libraries(${CMAKE_TEST_NAME} PUBLIC futures)

    if (FUTURES_TESTS_SMALL_POOL)
        target_compile_definitions(${CMAKE_TEST_NAME} INTERFACE FUTURES_DEFAULT_THREAD_POOL_SIZE=1)
    endif ()

    # Register with ctest
    add_test(NAME ${CMAKE_TEST_NAME} COMMAND ${CMAKE_TEST_NAME} 10)
endmacro()

######################################################
# Separate CMake tests                               #
######################################################
add_futures_stress_test(wait.cpp)
add_futures_stress_test(launch.cpp)