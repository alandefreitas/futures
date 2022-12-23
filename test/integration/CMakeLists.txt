#######################################################
### Integration tests                               ###
#######################################################
add_custom_target(
        make-integration-test-build-dirs

        # Create build dir
        COMMAND "${CMAKE_COMMAND}" -E make_directory ${CMAKE_CURRENT_BINARY_DIR}/header_only/build
        COMMAND "${CMAKE_COMMAND}" -E make_directory ${CMAKE_CURRENT_BINARY_DIR}/compiled/build
        COMMAND "${CMAKE_COMMAND}" -E make_directory ${CMAKE_CURRENT_BINARY_DIR}/cmake_subdir/build
        COMMAND "${CMAKE_COMMAND}" -E make_directory ${CMAKE_CURRENT_BINARY_DIR}/cmake_fetch/build
        COMMAND "${CMAKE_COMMAND}" -E make_directory ${CMAKE_CURRENT_BINARY_DIR}/cmake_package/build

        COMMENT "Make integration test build dirs"
        VERBATIM
)

add_custom_target(
        header-only-integration-check

        # Configure all
        COMMAND "${CMAKE_COMMAND}"
        -D CMAKE_CXX_COMPILER=${CMAKE_CXX_COMPILER} # build options
        -D CMAKE_CXX_STANDARD=${CMAKE_CXX_STANDARD}
        ${CMAKE_CURRENT_SOURCE_DIR}/header_only # source dir

        # Build all
        COMMAND "${CMAKE_COMMAND}"
        --build ${CMAKE_CURRENT_BINARY_DIR}/header_only/build

        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/header_only/build # build dir
        COMMENT "Check header-only integration"
        VERBATIM
)
add_dependencies(header-only-integration-check make-integration-test-build-dirs)

add_custom_target(
        compiled-integration-check

        # Configure all
        COMMAND "${CMAKE_COMMAND}"
        -D CMAKE_CXX_COMPILER=${CMAKE_CXX_COMPILER} # build options
        -D CMAKE_CXX_STANDARD=${CMAKE_CXX_STANDARD}
        ${CMAKE_CURRENT_SOURCE_DIR}/compiled # source dir

        # Build all
        COMMAND "${CMAKE_COMMAND}"
        --build ${CMAKE_CURRENT_BINARY_DIR}/compiled/build

        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/compiled/build # build dir
        COMMENT "Check compiled integration"
        VERBATIM
)
add_dependencies(compiled-integration-check make-integration-test-build-dirs)

add_custom_target(
        cmake_subdir-integration-check

        # Configure all
        COMMAND "${CMAKE_COMMAND}"
        -D CMAKE_CXX_COMPILER=${CMAKE_CXX_COMPILER} # build options
        -D CMAKE_CXX_STANDARD=${CMAKE_CXX_STANDARD}
        ${CMAKE_CURRENT_SOURCE_DIR}/cmake_subdir # source dir

        # Build all
        COMMAND "${CMAKE_COMMAND}"
        --build ${CMAKE_CURRENT_BINARY_DIR}/cmake_subdir/build

        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/cmake_subdir/build # build dir
        COMMENT "Check subdir integration"
        VERBATIM
)
add_dependencies(cmake_subdir-integration-check make-integration-test-build-dirs)

if (CMAKE_VERSION VERSION_GREATER_EQUAL 3.11.4)
    add_custom_target(
            cmake_fetch-integration-check

            # Configure all
            COMMAND "${CMAKE_COMMAND}"
            -D CMAKE_CXX_COMPILER=${CMAKE_CXX_COMPILER} # build options
            -D CMAKE_CXX_STANDARD=${CMAKE_CXX_STANDARD}
            ${CMAKE_CURRENT_SOURCE_DIR}/cmake_fetch # source dir

            # Build all
            COMMAND "${CMAKE_COMMAND}"
            --build ${CMAKE_CURRENT_BINARY_DIR}/cmake_fetch/build

            WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/cmake_fetch/build # build dir
            COMMENT "Check fetch integration"
            VERBATIM
    )
    add_dependencies(cmake_fetch-integration-check make-integration-test-build-dirs)
endif ()

add_custom_target(
        cmake_package-integration-check

        # Configure all
        COMMAND "${CMAKE_COMMAND}"
        -D CMAKE_CXX_COMPILER=${CMAKE_CXX_COMPILER} # build options
        -D CMAKE_CXX_STANDARD=${CMAKE_CXX_STANDARD}
        -D CMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
        -D CMAKE_CXX_COMPILER=${CMAKE_CXX_COMPILER}
        -D CMAKE_PREFIX_PATH=${CMAKE_INSTALL_PREFIX}
        ${CMAKE_CURRENT_SOURCE_DIR}/cmake_package # source dir

        # Build all
        COMMAND "${CMAKE_COMMAND}"
        --build ${CMAKE_CURRENT_BINARY_DIR}/cmake_package/build
        --config ${CMAKE_BUILD_TYPE} # win32 requires build type when building to match install library

        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/cmake_package/build # build dir
        COMMENT "Check package integration"
        VERBATIM
)
add_dependencies(cmake_package-integration-check make-integration-test-build-dirs)

