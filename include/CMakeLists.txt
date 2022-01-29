#######################################################
### Dependencies                                    ###
#######################################################
find_package(Threads)
find_package(Boost 1.78.0 CONFIG)
find_package(Asio 1.21.0 QUIET)
if (NOT Asio_FOUND)
    FetchContent_Declare(Asio
            GIT_REPOSITORY https://github.com/chriskohlhoff/asio.git
            GIT_TAG        asio-1-21-0
            )
    FetchContent_GetProperties(Asio)
    if (NOT Asio_POPULATED)
        FetchContent_Populate(Asio)
    endif ()
    add_library(asio::asio INTERFACE IMPORTED)
    message("Include asio from ${asio_SOURCE_DIR}/asio/include")
    set_target_properties(asio::asio PROPERTIES INTERFACE_INCLUDE_DIRECTORIES "${asio_SOURCE_DIR}/asio/include")
    set(Asio_FOUND ON)
endif ()

#######################################################
### Futures library                                 ###
#######################################################
# CONFIGURE_DEPENDS works on most generators now
file(GLOB_RECURSE FUTURES_HEADERS CONFIGURE_DEPENDS futures/**/*.hpp)
add_library(futures INTERFACE)

# Target aliases
# This emulates find_package when add_subdirectory has been used
add_library(futures::futures ALIAS futures)

# This library requires C++17
target_compile_features(futures INTERFACE cxx_std_17)

# Include directories (development and installation)
target_include_directories(futures
        INTERFACE $<BUILD_INTERFACE:${FUTURES_ROOT_DIR}/include>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
        )

# Compiler options
# MSVC compatibility
target_bigobj_options(futures)
target_utf8_options(futures)
target_nominmax_definition(futures)

if (BUILD_WITH_EXCEPTIONS)
    target_exception_options(futures)
endif ()

# Always enable warnings in development mode
maybe_target_pedantic_warnings(futures)

#######################################################
### Dependencies                                    ###
#######################################################
if (Boost_FOUND)
    target_link_libraries(futures INTERFACE Boost::boost)
endif ()
if (small_FOUND OR Small_FOUND)
    target_link_libraries(futures INTERFACE small::small)
endif ()
if (asio_FOUND OR Asio_FOUND)
    target_link_libraries(futures INTERFACE asio::asio)
endif ()
target_link_libraries(futures INTERFACE ${CMAKE_THREAD_LIBS_INIT})

###############################################################
### Set proper compile options if we already know about it  ###
###############################################################
if (FUTURES_PREFER_BOOST_DEPENDENCIES)
    target_compile_definitions(futures INTERFACE FUTURES_PREFER_BOOST_DEPENDENCIES)
endif ()
if (FUTURES_PREFER_STANDALONE_DEPENDENCIES)
    target_compile_definitions(futures INTERFACE FUTURES_PREFER_STANDALONE_DEPENDENCIES)
endif ()

###########################################################
### Single-header library                               ###
###########################################################
# Generate a single-header version of futures
if (FUTURES_BUILD_SINGLE_HEADER)
    set(SINGLE_HEADER_PATH "${CMAKE_CURRENT_BINARY_DIR}/single-header/futures/futures.hpp")
    add_custom_command(
            OUTPUT ${SINGLE_HEADER_PATH}
            COMMAND amalgamator
            --include_paths
            "${FUTURES_ROOT_DIR}/include"
            --entry_points
            "futures/futures.hpp"
            "futures/algorithm.hpp"
            --double_include
            --output
            "${SINGLE_HEADER_PATH}"
            --remove_leading_comments
            --show_progress
            DEPENDS amalgamator ${FUTURES_HEADERS}
            COMMENT "Creating single-header library"
            VERBATIM
    )
    add_custom_target(build_single_include DEPENDS ${SINGLE_HEADER_PATH})
    add_library(single-header-futures INTERFACE)
    add_dependencies(single-header-futures build_single_include)
    add_library(futures::single-header-futures ALIAS single-header-futures)

    target_include_directories(single-header-futures
            INTERFACE $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/single-header>
            $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/futures/single-header>
            )

    foreach (FUTURES_PROPERTY INTERFACE_COMPILE_FEATURES INTERFACE_COMPILE_OPTIONS INTERFACE_LINK_LIBRARIES)
        get_target_property(PROPERTY_VALUE futures ${FUTURES_PROPERTY})
        set_target_properties(single-header-futures PROPERTIES ${FUTURES_PROPERTY} "${PROPERTY_VALUE}")
    endforeach ()
endif ()

#######################################################
### Installer                                       ###
#######################################################
if (FUTURES_BUILD_INSTALLER)
    # Install targets
    if (FUTURES_BUILD_SINGLE_HEADER)
        set(INSTALL_TARGET_LIST futures single-header-futures)
    else()
        set(INSTALL_TARGET_LIST futures)
    endif()
    install(TARGETS ${INSTALL_TARGET_LIST}
            EXPORT futures-targets
            LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
            ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
            )

    # Install headers
    install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/futures
            DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
            FILES_MATCHING PATTERN "*.hpp"
            )

    if (FUTURES_BUILD_SINGLE_HEADER)
        install(FILES ${SINGLE_HEADER_PATH}
            DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/futures/single-header/futures
            )
    endif()

    # Install cmake script
    install(EXPORT futures-targets
            FILE futures-targets.cmake
            NAMESPACE futures::
            DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/futures
            )
endif ()
