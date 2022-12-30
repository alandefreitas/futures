#######################################################
### Stress Tests                                    ###
#######################################################
macro(add_futures_benchmark TEST_FILE)
    # Create a name for the test
    string(REPLACE "/" "_" CMAKE_TEST_NAME ${TEST_FILE})
    string(REPLACE ".cpp" "" CMAKE_TEST_NAME ${CMAKE_TEST_NAME})
    set(CMAKE_TEST_NAME bench_${CMAKE_TEST_NAME})

    # Create executable for test
    add_executable(${CMAKE_TEST_NAME} ${TEST_FILE})

    # Link futures
    target_link_libraries(${CMAKE_TEST_NAME} PUBLIC futures)
endmacro()

######################################################
# Separate CMake tests                               #
######################################################
add_futures_benchmark(eager_vs_deferred.cpp)