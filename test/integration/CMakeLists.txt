#######################################################
### Integration tests                               ###
#######################################################
add_custom_target(
        header-only-integration-check

        # Configure all
        COMMAND "${CMAKE_COMMAND}"
        -S ${CMAKE_CURRENT_SOURCE_DIR}/header_only
        -B ${CMAKE_CURRENT_BINARY_DIR}/header_only/build

        # Build all
        COMMAND "${CMAKE_COMMAND}"
        --build ${CMAKE_CURRENT_BINARY_DIR}/header_only/build

        COMMENT "Check header-only integration"
        VERBATIM
)

add_custom_target(
        compiled-integration-check

        # Configure all
        COMMAND "${CMAKE_COMMAND}"
        -S ${CMAKE_CURRENT_SOURCE_DIR}/compiled
        -B ${CMAKE_CURRENT_BINARY_DIR}/compiled/build

        # Build all
        COMMAND "${CMAKE_COMMAND}"
        --build ${CMAKE_CURRENT_BINARY_DIR}/compiled/build

        COMMENT "Check compiled integration"
        VERBATIM
)

add_custom_target(
        cmake_subdir-integration-check

        # Configure all
        COMMAND "${CMAKE_COMMAND}"
        -S ${CMAKE_CURRENT_SOURCE_DIR}/cmake_subdir
        -B ${CMAKE_CURRENT_BINARY_DIR}/cmake_subdir/build

        # Build all
        COMMAND "${CMAKE_COMMAND}"
        --build ${CMAKE_CURRENT_BINARY_DIR}/cmake_subdir/build

        COMMENT "Check subdir integration"
        VERBATIM
)

add_custom_target(
        cmake_fetch-integration-check

        # Configure all
        COMMAND "${CMAKE_COMMAND}"
        -S ${CMAKE_CURRENT_SOURCE_DIR}/cmake_fetch
        -B ${CMAKE_CURRENT_BINARY_DIR}/cmake_fetch/build

        # Build all
        COMMAND "${CMAKE_COMMAND}"
        --build ${CMAKE_CURRENT_BINARY_DIR}/cmake_fetch/build

        COMMENT "Check fetch integration"
        VERBATIM
)

add_custom_target(
        cmake_package-integration-check

        # Configure all
        COMMAND "${CMAKE_COMMAND}"
        -S ${CMAKE_CURRENT_SOURCE_DIR}/cmake_package
        -B ${CMAKE_CURRENT_BINARY_DIR}/cmake_package/build
        -D CMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
        -D CMAKE_C_COMPILER=${CMAKE_C_COMPILER}
        -D CMAKE_CXX_COMPILER=${CMAKE_CXX_COMPILER}
        -D CMAKE_PREFIX_PATH=${CMAKE_INSTALL_PREFIX}

        # Build all
        COMMAND "${CMAKE_COMMAND}"
        --build ${CMAKE_CURRENT_BINARY_DIR}/cmake_package/build
        --config ${CMAKE_BUILD_TYPE}

        COMMENT "Check package integration"
        VERBATIM
)

