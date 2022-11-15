#######################################################
### Futures                                         ###
#######################################################
cmake_minimum_required(VERSION 3.16)
project(
        futures
        VERSION 0.1.4
        DESCRIPTION "C++ Lock-Free Task Programming"
        HOMEPAGE_URL "https://alandefreitas.github.io/futures/"
)
set(FUTURES_ROOT_DIR ${CMAKE_CURRENT_SOURCE_DIR})
set(FUTURES_VERSION ${CMAKE_PROJECT_VERSION})
message(STATUS "${CMAKE_PROJECT_NAME} v${CMAKE_PROJECT_VERSION}: ${CMAKE_PROJECT_HOMEPAGE_URL}")
include(cmake/futures/in_source_guard.cmake)
include(cmake/futures/variables.cmake)
if (BUILD_SHARED_LIBS AND WIN32)
    set(LIBRARY_OUTPUT_PATH ${CMAKE_BINARY_DIR}/targets)
    set(EXECUTABLE_OUTPUT_PATH ${CMAKE_BINARY_DIR}/targets)
endif()

#######################################################
### Options                                         ###
#######################################################
# What to build
option(FUTURES_BUILD_INSTALLER "Build installer target" ${MASTER_PROJECT})
option(FUTURES_BUILD_PACKAGE "Build package" ${MASTER_PROJECT})

# How to build
option(FUTURES_USE_FIND_PACKAGE "Do not use find_package to find dependencies" ${FUTURES_NOT_CROSSCOMPILING})
option(FUTURES_USE_FETCH_CONTENT "Do not use FetchContent to fetch dependencies" ${FUTURES_NOT_CROSSCOMPILING})
option(FUTURES_PREFER_STANDALONE_ASIO "Prefer standalone Asio if available" OFF)
option(FUTURES_PREFER_BOOST "Prefer Boost.Asio if available" OFF)
option(FUTURES_PREFER_BUNDLED "Prefer bundled dependencies" OFF)
option(FUTURES_BUILD_WITH_EXCEPTIONS "Add compiler flags to use exceptions if needed." ON)

# Development options
include(cmake/futures/dev-options.cmake)

#######################################################
### Libraries                                       ###
#######################################################
include(cmake/futures/dependencies.cmake)
add_subdirectory(include)
add_subdirectory(src)

#######################################################
### Installer                                       ###
#######################################################
if (FUTURES_BUILD_INSTALLER)
    set(INSTALL_TARGET_LIST headers futures)
    if (Asio_FETCHED)
        list(APPEND INSTALL_TARGET_LIST asio)
    endif()

    # Install targets
    install(TARGETS ${INSTALL_TARGET_LIST}
            EXPORT futures-targets
            LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
            ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
            )

    # Install headers
    install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/include/futures
            DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
            FILES_MATCHING PATTERN "*.[hic]pp"
            )

    # If Asio had to be fetched, install it
    # This installation has no targets, since those will be defined by
    # FindAsio.cmake
    if (Asio_FETCHED)
        install(DIRECTORY ${asio_SOURCE_DIR}/asio/include
                DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/..
                FILES_MATCHING PATTERN "*.[hic]pp"
                )

        install(FILES ${asio_SOURCE_DIR}/asio/LICENSE_1_0.txt
                DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/asio
                )
    endif()

    # Install cmake targets script
    install(EXPORT futures-targets
            FILE futures-targets.cmake
            NAMESPACE futures::
            DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/futures
            )

    # Set variable where the cmake config is
    # https://cliutils.gitlab.io/modern-cmake/chapters/install/installing.html
    set(CONFIG_INSTALL_DIR ${CMAKE_INSTALL_LIBDIR}/cmake/futures)
    message(STATUS "${CMAKE_PROJECT_NAME} install directory: ${CMAKE_INSTALL_PREFIX}")
    message(STATUS "${CMAKE_PROJECT_NAME} library install directory: ${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_LIBDIR}")

    # Create futures-config-version.cmake
    if (CMAKE_PROJECT_VERSION VERSION_LESS 1.0.0)
        set(compatibility_mode SameMajorVersion)
    else ()
        set(compatibility_mode SameMinorVersion)
    endif ()

    write_basic_package_version_file(
            futures-config-version.cmake
            VERSION ${PACKAGE_VERSION}
            COMPATIBILITY ${compatibility_mode}
    )

    # Install futures-config-version.cmake
    install(FILES ${CMAKE_CURRENT_BINARY_DIR}/futures-config-version.cmake
            DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/futures)

    # Create futures-config.cmake
    set(INCLUDE_INSTALL_DIR include/ )
    set(LIB_INSTALL_DIR lib/ )
    configure_package_config_file(
            ${CMAKE_CURRENT_SOURCE_DIR}/futures-config.cmake.in # input file
            ${CMAKE_CURRENT_BINARY_DIR}/futures-config.cmake    # output file
            INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/futures
            PATH_VARS CMAKE_INSTALL_LIBDIR INCLUDE_INSTALL_DIR LIB_INSTALL_DIR
    )

    # Install the file futures-config.cmake
    install(FILES ${CMAKE_CURRENT_BINARY_DIR}/futures-config.cmake
            DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/futures)

    # Install FindAsio.cmake
    install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/cmake/FindAsio.cmake
            DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/futures)
endif ()

#######################################################
### Packages                                        ###
#######################################################
if (FUTURES_BUILD_INSTALLER AND FUTURES_BUILD_PACKAGE)
    # Set the cpack variables
    # https://cliutils.gitlab.io/modern-cmake/chapters/install/packaging.html

    # The most common cpack variables
    set(CPACK_PACKAGE_VENDOR "futures")
    set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "futures: C++ futures containers")
    set(CPACK_PACKAGE_VERSION_MAJOR ${PROJECT_VERSION_MAJOR})
    set(CPACK_PACKAGE_VERSION_MINOR ${PROJECT_VERSION_MINOR})
    set(CPACK_PACKAGE_VERSION_PATCH ${PROJECT_VERSION_PATCH})
    set(CPACK_RESOURCE_FILE_LICENSE "${FUTURES_ROOT_DIR}/LICENSE")
    set(CPACK_RESOURCE_FILE_README "${FUTURES_ROOT_DIR}/README.md")

    # Set CPACK_SOURCE_IGNORE_FILES with files source packages shouldn't install
    # We get these from .gitignore to avoid redundancy
    FILE(READ .gitignore GITIGNORE_CONTENTS)
    STRING(REGEX REPLACE ";" "\\\\;" GITIGNORE_CONTENTS "${GITIGNORE_CONTENTS}")
    STRING(REGEX REPLACE "\n" ";" GITIGNORE_CONTENTS "${GITIGNORE_CONTENTS}")
    set(CPACK_SOURCE_IGNORE_FILES ${GITIGNORE_CONTENTS})

    # Always include CPack at last
    include(CPack)
endif ()

#######################################################
### Developer mode                                  ###
#######################################################
if (NOT FUTURES_DEVELOPER_MODE)
    add_subdirectory(test/integration)
    return()
elseif (NOT MASTER_PROJECT)
    message(
            AUTHOR_WARNING
            "Developer mode is intended for developers"
    )
endif ()

add_subdirectory(dev-tools)

if (FUTURES_BUILD_TESTS)
    enable_testing()
    add_subdirectory(test)
endif ()

if (FUTURES_BUILD_DOCS)
    add_subdirectory(docs)
endif()

if (FUTURES_BUILD_COVERAGE_REPORT)
    include(cmake/futures/coverage-report.cmake)
endif()

if (FUTURES_BUILD_LINT)
    include(cmake/futures/lint-targets.cmake)
endif()

if (FUTURES_BUILD_EXAMPLES)
    add_subdirectory(example)
endif ()