#######################################################
### Dependencies                                    ###
#######################################################
if (FUTURES_USE_FIND_PACKAGE)
    find_package(Catch2 2.13.8 CONFIG)
endif ()

if (Catch2_FOUND)
    include(${Catch2_DIR}/Catch.cmake)
elseif (FUTURES_USE_FETCH_CONTENT)
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
else ()
    # Bundled Catch2
    add_library(Catch2 INTERFACE)
    add_library(Catch2::Catch2 ALIAS Catch2)
    target_include_directories(Catch2 INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})
    include(${CMAKE_CURRENT_SOURCE_DIR}/catch2/contrib/Catch.cmake)
endif ()

# Create a common catch main for all tests
add_library(catch_main STATIC catch_main.cpp)
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
    list(APPEND FUTURES_TESTS_SOURCE ${TEST_FILE})

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
    if (FUTURES_CATCH2_REPORTER STREQUAL "console" OR BUILD_SHARED_LIBS)
        # This is the default normally used for local tests
        catch_discover_tests(
                ${CMAKE_TEST_NAME}
                PROPERTIES
                ENVIRONMENT "detect_invalid_pointer_pairs=2"
        )
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
                PROPERTIES
                ENVIRONMENT "detect_invalid_pointer_pairs=2"
        )
    endif ()
endmacro()

######################################################
# Separate CMake tests                               #
######################################################
file(GLOB_RECURSE UNIT_TESTS RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} CONFIGURE_DEPENDS **.cpp )
list(REMOVE_ITEM UNIT_TESTS catch_main.cpp)
foreach (UNIT_TEST ${UNIT_TESTS})
    add_futures_test(${UNIT_TEST})
endforeach()

#####################################################
# All tests as a single target                      #
#####################################################
if (FUTURES_BUILD_SINGLE_TARGET_TESTS)
    add_executable(ut_futures_all catch_main.cpp ${FUTURES_TESTS_SOURCE})
    target_link_libraries(ut_futures_all PUBLIC futures::futures Catch2::Catch2)
    target_compile_features(ut_futures_all PUBLIC cxx_std_17)
    target_compile_definitions(ut_futures_all PUBLIC TEST_CASE_PREFIX="")
endif ()