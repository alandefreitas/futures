###########################################################
### Amalgamator application                             ###
###########################################################
add_executable(amalgamator main.cpp)
target_compile_features(amalgamator PUBLIC cxx_std_17)

###########################################################
### Single-header library                               ###
###########################################################
# Generate a single-header version of futures
if (FUTURES_BUILD_SINGLE_HEADER)
    set(SINGLE_HEADER_PATH "${PROJECT_BINARY_DIR}/include/single_header/futures/futures.hpp")
    add_custom_command(
            OUTPUT ${SINGLE_HEADER_PATH}

            COMMAND amalgamator

            # Files
            --include_paths "${FUTURES_ROOT_DIR}/include"
            --entry_points "futures/futures.hpp" "futures/algorithm.hpp"
            --output  "${SINGLE_HEADER_PATH}"

            # Amalgamator options
            --remove_leading_comments
            --show_progress

            # CMake command options
            DEPENDS amalgamator ${FUTURES_HEADERS}
            COMMENT "Creating single_header library"
            VERBATIM
    )
    add_custom_target(build_single_include DEPENDS ${SINGLE_HEADER_PATH})

    add_library(single_header INTERFACE)
    add_library(futures::single_header ALIAS single_header)

    add_dependencies(single_header build_single_include)
    target_compile_features(single_header INTERFACE cxx_std_17)


    target_include_directories(single_header
            INTERFACE $<BUILD_INTERFACE:${PROJECT_BINARY_DIR}/include/single_header>
            $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/futures/single_header>
            )

    # Compiler options
    # MSVC compatibility
    target_bigobj_options(single_header)
    target_utf8_options(single_header)
    if (BUILD_WITH_EXCEPTIONS)
        target_exception_options(single_header)
    endif ()

    # Always enable warnings in development mode
    maybe_target_pedantic_warnings(single_header)

    # Get properties from main futures target
    if (Boost_FOUND)
        target_link_libraries(single_header INTERFACE Boost::boost)
    endif ()
    if (small_FOUND OR Small_FOUND)
        target_link_libraries(single_header INTERFACE small::small)
    endif ()
    if (asio_FOUND OR Asio_FOUND)
        target_link_libraries(single_header INTERFACE asio::asio)
    endif ()
    target_link_libraries(single_header INTERFACE ${CMAKE_THREAD_LIBS_INIT})

    if (FUTURES_PREFER_BOOST_DEPENDENCIES)
        target_compile_definitions(single_header INTERFACE FUTURES_PREFER_BOOST_DEPENDENCIES)
    endif ()
    if (FUTURES_PREFER_STANDALONE_DEPENDENCIES)
        target_compile_definitions(single_header INTERFACE FUTURES_PREFER_STANDALONE_DEPENDENCIES)
    endif ()
endif ()
