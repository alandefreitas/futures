#######################################################
### Futures library                                 ###
#######################################################
# CONFIGURE_DEPENDS works on most generators now
file(GLOB_RECURSE FUTURES_HEADERS CONFIGURE_DEPENDS futures/**/*.h)
add_library(futures INTERFACE)

# Target aliases
# This emulates find_package when add_subdirectory has been used
add_library(futures::futures ALIAS futures)

# This library requires C++17
target_compile_features(futures INTERFACE cxx_std_17)

# Include directories (development and installation)
target_include_directories(futures
        INTERFACE $<BUILD_INTERFACE:${FUTURES_ROOT_DIR}/source>
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
endif()
if (small_FOUND OR Small_FOUND)
    target_link_libraries(futures INTERFACE small::small)
endif()
if (asio_FOUND OR Asio_FOUND)
    target_link_libraries(futures INTERFACE asio::asio)
endif()
target_link_libraries(futures INTERFACE ${CMAKE_THREAD_LIBS_INIT})

###############################################################
### Set proper compile options if we already know about it  ###
###############################################################
if (FUTURES_PREFER_BOOST_DEPENDENCIES)
    target_compile_definitions(futures INTERFACE FUTURES_PREFER_BOOST_DEPENDENCIES)
endif()
if (FUTURES_PREFER_STANDALONE_DEPENDENCIES)
    target_compile_definitions(futures INTERFACE FUTURES_PREFER_STANDALONE_DEPENDENCIES)
endif()

###########################################################
### Single-header library                               ###
###########################################################
# Generate a single-header version of futures
set(SINGLE_HEADER_PATH "${FUTURES_ROOT_DIR}/source/single-header/futures.h")
add_custom_command(
        OUTPUT ${SINGLE_HEADER_PATH}
        COMMAND amalgamator
        --include_paths
        "${FUTURES_ROOT_DIR}/source"
        --entry_points
        "futures/futures.h"
        "futures/algorithm.h"
        --double_include
        --output
        "${FUTURES_ROOT_DIR}/source/single-header/futures/futures.h"
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
        INTERFACE $<BUILD_INTERFACE:${FUTURES_ROOT_DIR}/source/single-header>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/single-header>
        )

foreach(FUTURES_PROPERTY INTERFACE_COMPILE_FEATURES INTERFACE_COMPILE_OPTIONS INTERFACE_LINK_LIBRARIES)
    get_target_property(PROPERTY_VALUE futures ${FUTURES_PROPERTY})
    set_target_properties(single-header-futures PROPERTIES ${FUTURES_PROPERTY} "${PROPERTY_VALUE}")
endforeach()

###########################################################
### Target with separate compilation (not header-only)  ###
###########################################################
# This is a version of futures where ASIO is not header-only
add_library(futures-compiled
        futures-compiled/src.cpp
        )
target_link_libraries(futures-compiled PUBLIC futures)
target_compile_definitions(futures-compiled PUBLIC FUTURES_SEPARATE_COMPILATION)
add_library(futures::futures-compiled ALIAS futures-compiled)



#######################################################
### Installer                                       ###
#######################################################
if (FUTURES_BUILD_INSTALLER)
    # Install targets
    install(TARGETS futures
            EXPORT futures-targets
            LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
            ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
            )

    # Install headers
    install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/futures
            DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
            FILES_MATCHING PATTERN "*.h"
            )

    # Install cmake script
    install(EXPORT futures-targets
            FILE futures-targets.cmake
            NAMESPACE futures::
            DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/futures
            )
endif ()
