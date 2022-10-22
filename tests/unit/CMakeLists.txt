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
add_library(catch_main catch_main.cpp)
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
macro(add_futures_test TEST_FILE)
    # Create a name for the test
    string(REPLACE "/" "_" CMAKE_TEST_NAME ${TEST_FILE})
    string(REPLACE ".cpp" "" CMAKE_TEST_NAME ${CMAKE_TEST_NAME})
    set(CMAKE_TEST_NAME ut_${CMAKE_TEST_NAME})

    # Create test
    add_executable(${CMAKE_TEST_NAME} ${TEST_FILE})
    target_link_libraries(${CMAKE_TEST_NAME} PUBLIC futures catch_main)

    # Test compile options
    if (GCC)
        # A GCC bug on some tests that conflict with boost.container
        target_compile_options(${CMAKE_TEST_NAME} INTERFACE -Wno-error=uninitialized -Wno-error=maybe-uninitialized)
    endif ()
    if (FUTURES_TESTS_SMALL_POOL)
        target_compile_definitions(${CMAKE_TEST_NAME} INTERFACE FUTURES_DEFAULT_THREAD_POOL_SIZE=1)
    endif ()

    # Register with ctest
    if (FUTURES_CATCH2_REPORTER STREQUAL "console")
        # This is the default normally used for local tests
        catch_discover_tests(${CMAKE_TEST_NAME})
    else ()
        # This is custom reporter (usually junit) used in CI
        # The junit-report action provides feedback on the tests
        if (NOT EXISTS ${CMAKE_CURRENT_BINARY_DIR}/reports)
            file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/reports)
        endif ()
        catch_discover_tests(
                ${CMAKE_TEST_NAME}
                REPORTER ${FUTURES_CATCH2_REPORTER}
                OUTPUT_DIR ${CMAKE_CURRENT_BINARY_DIR}/reports
                OUTPUT_SUFFIX ".${FUTURES_CATCH2_REPORTER}"
        )
    endif ()
endmacro()

######################################################
# Separate CMake tests                               #
######################################################
add_futures_test(adaptor.cpp)
add_futures_test(algorithm.cpp)
add_futures_test(await.cpp)
add_futures_test(config.cpp)
add_futures_test(executor.cpp)
add_futures_test(future.cpp)
add_futures_test(future_error.cpp)
add_futures_test(future_options.cpp)
add_futures_test(future_options_args.cpp)
add_futures_test(futures.cpp)
add_futures_test(is_ready.cpp)
add_futures_test(launch.cpp)
add_futures_test(packaged_task.cpp)
add_futures_test(promise.cpp)
add_futures_test(stop_token.cpp)
add_futures_test(wait_for_all.cpp)
add_futures_test(wait_for_any.cpp)
add_futures_test(adaptor/bind_executor_to_lambda.cpp)
add_futures_test(adaptor/make_ready_future.cpp)
add_futures_test(adaptor/then.cpp)
add_futures_test(adaptor/when_all.cpp)
add_futures_test(adaptor/when_any.cpp)
add_futures_test(adaptor/detail/continue.cpp)
add_futures_test(algorithm/all_of.cpp)
add_futures_test(algorithm/any_of.cpp)
add_futures_test(algorithm/count.cpp)
add_futures_test(algorithm/count_if.cpp)
add_futures_test(algorithm/find.cpp)
add_futures_test(algorithm/find_if.cpp)
add_futures_test(algorithm/find_if_not.cpp)
add_futures_test(algorithm/for_each.cpp)
add_futures_test(algorithm/none_of.cpp)
add_futures_test(algorithm/policies.cpp)
add_futures_test(algorithm/reduce.cpp)
add_futures_test(algorithm/comparisons/compare_three_way.cpp)
add_futures_test(algorithm/comparisons/equal_to.cpp)
add_futures_test(algorithm/comparisons/greater.cpp)
add_futures_test(algorithm/comparisons/greater_equal.cpp)
add_futures_test(algorithm/comparisons/less.cpp)
add_futures_test(algorithm/comparisons/less_equal.cpp)
add_futures_test(algorithm/comparisons/not_equal_to.cpp)
add_futures_test(algorithm/partitioner/partitioner.cpp)
add_futures_test(algorithm/traits/binary_invoke_algorithm.cpp)
add_futures_test(algorithm/traits/is_assignable_from.cpp)
add_futures_test(algorithm/traits/is_constructible_from.cpp)
add_futures_test(algorithm/traits/is_convertible_to.cpp)
add_futures_test(algorithm/traits/is_copyable.cpp)
add_futures_test(algorithm/traits/is_default_initializable.cpp)
add_futures_test(algorithm/traits/is_derived_from.cpp)
add_futures_test(algorithm/traits/is_equality_comparable.cpp)
add_futures_test(algorithm/traits/is_equality_comparable_with.cpp)
add_futures_test(algorithm/traits/is_forward_iterator.cpp)
add_futures_test(algorithm/traits/is_incrementable.cpp)
add_futures_test(algorithm/traits/is_indirectly_binary_invocable.cpp)
add_futures_test(algorithm/traits/is_indirectly_readable.cpp)
add_futures_test(algorithm/traits/is_indirectly_unary_invocable.cpp)
add_futures_test(algorithm/traits/is_input_iterator.cpp)
add_futures_test(algorithm/traits/is_input_or_output_iterator.cpp)
add_futures_test(algorithm/traits/is_input_range.cpp)
add_futures_test(algorithm/traits/is_movable.cpp)
add_futures_test(algorithm/traits/is_move_constructible.cpp)
add_futures_test(algorithm/traits/is_partially_ordered_with.cpp)
add_futures_test(algorithm/traits/is_range.cpp)
add_futures_test(algorithm/traits/is_regular.cpp)
add_futures_test(algorithm/traits/is_semiregular.cpp)
add_futures_test(algorithm/traits/is_sentinel_for.cpp)
add_futures_test(algorithm/traits/is_swappable.cpp)
add_futures_test(algorithm/traits/is_totally_ordered.cpp)
add_futures_test(algorithm/traits/is_totally_ordered_with.cpp)
add_futures_test(algorithm/traits/is_weakly_equality_comparable.cpp)
add_futures_test(algorithm/traits/is_weakly_incrementable.cpp)
add_futures_test(algorithm/traits/iter_difference.cpp)
add_futures_test(algorithm/traits/iter_reference.cpp)
add_futures_test(algorithm/traits/iter_rvalue_reference.cpp)
add_futures_test(algorithm/traits/iter_value.cpp)
add_futures_test(algorithm/traits/iterator.cpp)
add_futures_test(algorithm/traits/range_value.cpp)
add_futures_test(algorithm/traits/remove_cvref.cpp)
add_futures_test(algorithm/traits/unary_invoke_algorithm.cpp)
add_futures_test(algorithm/traits/value_cmp_algorithm.cpp)
add_futures_test(executor/default_executor.cpp)
add_futures_test(executor/inline_executor.cpp)
add_futures_test(executor/is_executor.cpp)
add_futures_test(executor/new_thread_executor.cpp)
add_futures_test(traits/future_value.cpp)
add_futures_test(traits/has_executor.cpp)
add_futures_test(traits/has_ready_notifier.cpp)
add_futures_test(traits/has_stop_token.cpp)
add_futures_test(traits/is_always_deferred.cpp)
add_futures_test(traits/is_continuable.cpp)
add_futures_test(traits/is_future.cpp)
add_futures_test(traits/is_shared_future.cpp)
add_futures_test(traits/is_stoppable.cpp)
add_futures_test(detail/utility/compressed_tuple.cpp)
add_futures_test(detail/container/queue.cpp)
add_futures_test(detail/operation_state.cpp)

#####################################################
# All tests as a single target                      #
#####################################################
if (FUTURES_BUILD_SINGLE_TARGET_TESTS)
    file(GLOB_RECURSE FUTURES_TESTS_SOURCE CONFIGURE_DEPENDS ./*.cpp)
    add_executable(ut_futures_all catch_main.cpp ${FUTURES_TESTS_SOURCE})
    target_link_libraries(ut_futures_all PUBLIC futures::futures Catch2::Catch2)
    target_compile_features(ut_futures_all PUBLIC cxx_std_17)
    target_compile_definitions(ut_futures_all PUBLIC TEST_CASE_PREFIX="")
endif ()