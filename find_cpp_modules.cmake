#######################################################
### cpp_modules hints                               ###
#######################################################
# If user has a cpp_modules directory in their project but the user didn't
# set the hint paths properly (i.e.: no toolchain / no manager / no env variables),
# we need some helper functions to identify the default configuration and
# identify these extra hint paths.
include(cmake/functions/cpp_modules.cmake)

#######################################################
### C++: Threads                                    ###
#######################################################
find_package(Threads)

#######################################################
### Boost                                           ###
#######################################################
# We look for Boost and standalone dependencies, although we only need one of them.
# All dependencies are also header only, so they are all also made available to
# the interface target. The user can later choose what to use.
# When boost is not found, we do not provide an alternative FetchContents because
# boost is a very large library.
set(boost_VERSION_LOCK 1.76.0)
set(boost_VERSION_REQUIREMENT ^1.76.0)
set_local_module_hints(boost ${boost_VERSION_LOCK} ${boost_VERSION_REQUIREMENT})

# Look for lock version
if (NOT Boost_FOUND)
    semver_split(${boost_VERSION_LOCK} boost_VERSION_LOCK)
    find_package(Boost ${boost_VERSION_LOCK_CORE} QUIET COMPONENTS ${boost_FIND_COMPONENTS} CONFIG)
endif ()

# Look for any version that matches our requirements
if (NOT Boost_FOUND)
    find_package(Boost QUIET COMPONENTS ${boost_FIND_COMPONENTS} CONFIG)
    if (Boost_FOUND AND Boost_VERSION AND NOT DEFINED ENV{Boost_ROOT})
        semver_requirements_compatible(${Boost_VERSION} ${Boost_VERSION_REQUIREMENT} ok)
        if (NOT ok)
            set(Boost_FOUND FALSE)
        endif ()
    endif ()
endif ()

if (Boost_FOUND)
    version_requirement_message(boost
            VERSION_FOUND ${Boost_VERSION}
            VERSION_LOCK ${boost_VERSION_LOCK}
            VERSION_REQUIREMENTS ${boost_VERSION_REQUIREMENT}
            PREFIX_HINT ${boost_PREFIX_HINT})
else()
    message(STATUS "boost (${boost_VERSION_REQUIREMENT}) not found - Optional dependency")
endif()

#######################################################
### ASIO                                            ###
#######################################################
# ASIO is a header-only library with no install script or CMake-config script
# This means all files should just be downloaded directly to the install directory
# and the FindAsio.cmake script should look for these headers.

# Set local hints, if any
set(Asio_VERSION_LOCK 1.20.0)
set(Asio_VERSION_REQUIREMENT ^1.20.0)
set_local_module_hints(asio ${Asio_VERSION_LOCK} ${Asio_VERSION_REQUIREMENT})

# CMake options and hints
set(Asio_ROOT_DIR ${Asio_ROOT})
set(Asio_FIND_COMPONENTS)

# Look for lock version
if (NOT Asio_FOUND)
    semver_split(${Asio_VERSION_LOCK} Asio_VERSION_LOCK)
    find_package(Asio ${Asio_VERSION_LOCK_CORE} COMPONENTS ${Asio_FIND_COMPONENTS} QUIET)
endif ()

# Look for any version that matches our requirements
if (NOT Asio_FOUND)
    find_package(Asio QUIET)
    if (Asio_FOUND AND Asio_VERSION AND NOT DEFINED ENV{Asio_ROOT})
        semver_requirements_compatible(${Asio_VERSION} ${Asio_VERSION_REQUIREMENT} COMPONENTS ${Asio_FIND_COMPONENTS} ok)
        if (NOT ok)
            set(Asio_FOUND FALSE)
        endif ()
    endif ()
endif ()

# Fetch Asio if we couldn't find a valid version
if (NOT Asio_FOUND)
    # Fallback to FetchContent and then find_package again
    if (EXISTS ${Asio_SOURCE_HINT})
        message("Sources for asio found...")
        file(GLOB SOURCE_DIR_GLOB ${Asio_SOURCE_HINT}/*)
        list(LENGTH SOURCE_DIR_GLOB RES_LEN)
        if (RES_LEN EQUAL 0)
            message("asio source directory is empty...")
            file(REMOVE ${Asio_SOURCE_HINT})
        else()
            set(Asio_SOURCE_DIR ${Asio_SOURCE_HINT})
            set(Asio_BINARY_DIR ${Asio_BINARY_HINT})
        endif()
    endif()

    if (NOT Asio_SOURCE_DIR)
        message("Downloading asio...")
        FetchContent_Declare(Asio
                URL https://sourceforge.net/projects/asio/files/asio/1.20.0%20%28Stable%29/asio-1.20.0.tar.bz2/download
                SOURCE_DIR ${Asio_SOURCE_HINT}
                BINARY_DIR ${Asio_BINARY_HINT}
                )

        # Check if already populated
        FetchContent_GetProperties(Asio)

        if (NOT Asio_POPULATED)
            # Download files
            FetchContent_Populate(Asio)
        endif ()
    endif ()

    # No configure step
    # No build step
    # Run install step: copying relevant header files
    message("Installing asio...")
    if (NOT EXISTS ${Asio_INSTALL_HINT})
        file(MAKE_DIRECTORY ${Asio_INSTALL_HINT})
    endif ()
    file(INSTALL ${Asio_SOURCE_HINT}/include DESTINATION ${Asio_INSTALL_HINT})

    # Find package again
    # ASIO requires a custom FindAsio.cmake
    message("Finding asio...")
    set(ENV{Asio_ROOT} ${Asio_INSTALL_HINT})
    find_package(Asio COMPONENTS ${Asio_FIND_COMPONENTS} REQUIRED)
endif ()
version_requirement_message(asio
        VERSION_FOUND ${Asio_VERSION}
        VERSION_LOCK ${Asio_VERSION_LOCK}
        VERSION_REQUIREMENTS ${Asio_VERSION_REQUIREMENT}
        PREFIX_HINT ${Asio_PREFIX_HINT}
        )

#######################################################
### Dev Dependencies                                ###
#######################################################
if (NOT FUTURES_DEV_BUILD)
    return()
endif ()

#######################################################
### Catch2                                          ###
#######################################################
set(catch2_VERSION_LOCK 2.13.6)
set(catch2_VERSION_REQUIREMENT ^2.0.0)
set_local_module_hints(catch2 ${catch2_VERSION_LOCK} ${catch2_VERSION_REQUIREMENT})

# Look for lock version
if (NOT Catch2_FOUND)
    semver_split(${catch2_VERSION_LOCK} catch2_VERSION_LOCK)
    find_package(Catch2 ${catch2_VERSION_LOCK_CORE} QUIET CONFIG)
endif ()

# Look for any version that matches our requirements
if (NOT Catch2_FOUND)
    find_package(Catch2 QUIET CONFIG)
    if (Catch2_FOUND AND catch2_VERSION AND NOT DEFINED ENV{catch2_ROOT})
        semver_requirements_compatible(${catch2_VERSION} ${catch2_VERSION_REQUIREMENT} ok)
        if (NOT ok)
            set(Catch2_FOUND FALSE)
        endif ()
    endif ()
endif ()

# Fetch catch2 if we couldn't find a valid version
if (NOT Catch2_FOUND)
    # Fallback to FetchContent and then find_package again
    message("Downloading catch2...")
    FetchContent_Declare(catch2
            URL https://github.com/catchorg/Catch2/archive/refs/tags/v2.13.6.zip
            SOURCE_DIR ${catch2_SOURCE_HINT}
            BINARY_DIR ${catch2_BINARY_HINT}
            )

    # Check if already populated
    FetchContent_GetProperties(catch2)

    if (NOT catch2_POPULATED)
        # Download files
        FetchContent_Populate(catch2)

        # Run configure step
        execute_process(COMMAND "${CMAKE_COMMAND}"
                # CMake options
                -G "${CMAKE_GENERATOR}"
                -DCMAKE_CXX_COMPILER=${CMAKE_CXX_COMPILER}
                -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
                -DBUILD_SHARED_LIBS=${BUILD_SHARED_LIBS}
                # CMake install prefix
                -DCMAKE_INSTALL_PREFIX=${catch2_INSTALL_HINT}
                # Allow looking for <package>_ROOT
                -DCMAKE_POLICY_DEFAULT_CMP0074=NEW
                # Package options
                -DCATCH_USE_VALGRIND=OFF # "Perform SelfTests with Valgrind"
                -DCATCH_BUILD_TESTING=OFF # "Build SelfTest project"
                -DCATCH_BUILD_EXAMPLES=OFF # "Build documentation examples"
                -DCATCH_BUILD_EXTRA_TESTS=OFF # "Build extra tests"
                -DCATCH_BUILD_STATIC_LIBRARY=OFF # "Builds static library from the main implementation. EXPERIMENTAL"
                -DCATCH_ENABLE_COVERAGE=OFF # "Generate coverage for codecov.io"
                -DCATCH_ENABLE_WERROR=OFF # "Enable all warnings as errors"
                -DCATCH_INSTALL_DOCS=OFF # "Install documentation alongside library"
                -DCATCH_INSTALL_HELPERS=ON # "Install contrib alongside library"
                # Source dir
                ${catch2_SOURCE_DIR}
                # Build dir
                WORKING_DIRECTORY "${catch2_BINARY_DIR}"
                )

        # Run build step
        execute_process(COMMAND "${CMAKE_COMMAND}" --build .
                WORKING_DIRECTORY "${catch2_BINARY_DIR}"
                )

        # Run install step
        execute_process(COMMAND "${CMAKE_COMMAND}" --install .
                WORKING_DIRECTORY "${catch2_BINARY_DIR}"
                )

        # Find package again
        set(ENV{catch2_ROOT} ${catch2_INSTALL_HINT})
        find_package(Catch2 REQUIRED CONFIG)
    endif ()
endif ()
version_requirement_message(catch2
        VERSION_FOUND ${catch2_VERSION}
        VERSION_LOCK ${catch2_VERSION_LOCK}
        VERSION_REQUIREMENTS ${catch2_VERSION_REQUIREMENT}
        PREFIX_HINT ${catch2_PREFIX_HINT})

