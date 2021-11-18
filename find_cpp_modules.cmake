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
### Small containers                                ###
#######################################################
set(small_VERSION_LOCK 0.1.1)
set(small_VERSION_REQUIREMENT ^0.1.1)
set_local_module_hints(small ${small_VERSION_LOCK} ${small_VERSION_REQUIREMENT})

# Look for lock version
if (NOT small_FOUND)
    semver_split(${small_VERSION_LOCK} small_VERSION_LOCK)
    find_package(small ${small_VERSION_LOCK_CORE} QUIET CONFIG)
endif ()

# Look for any version that matches our requirements
if (NOT small_FOUND)
    find_package(small QUIET CONFIG)
    if (small_FOUND AND small_VERSION AND NOT DEFINED ENV{small_ROOT})
        semver_requirements_compatible(${small_VERSION} ${small_VERSION_REQUIREMENT} ok)
        if (NOT ok)
            set(small_FOUND FALSE)
        endif ()
    endif ()
endif ()

# Fetch small if we couldn't find a valid version
if (NOT small_FOUND)
    # Fallback to FetchContent and then find_package again
    if (EXISTS ${small_SOURCE_HINT})
        message("Sources for small found...")
        set(small_SOURCE_DIR ${small_SOURCE_HINT})
        set(small_BINARY_DIR ${small_BINARY_HINT})
    else ()
        message("Downloading small...")
        FetchContent_Declare(small
                URL https://github.com/alandefreitas/small/archive/refs/tags/v0.1.1.tar.gz
                SOURCE_DIR ${small_SOURCE_HINT}
                BINARY_DIR ${small_BINARY_HINT}
                )

        # Check if already populated
        FetchContent_GetProperties(small)

        if (NOT small_POPULATED)
            # Download files
            FetchContent_Populate(small)
        endif ()
    endif ()

    # Run configure step
    message("Configuring small...")
    if (NOT EXISTS ${small_BINARY_DIR})
        file(MAKE_DIRECTORY ${small_BINARY_DIR})
    endif ()
    # Run configure step
    execute_process(COMMAND "${CMAKE_COMMAND}"
            # CMake options
            -G "${CMAKE_GENERATOR}"
            -DCMAKE_CXX_COMPILER=${CMAKE_CXX_COMPILER}
            -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
            -DBUILD_SHARED_LIBS=${BUILD_SHARED_LIBS}
            # CMake install prefix
            -DCMAKE_INSTALL_PREFIX=${small_INSTALL_HINT}
            # Package options
            -DSMALL_DEV_BUILD=OFF
            -DSMALL_BUILD_EXAMPLES=OFF
            -DSMALL_BUILD_TESTS=OFF
            -DSMALL_BUILD_TESTS_WITH_PCH=OFF
            -DSMALL_BUILD_INSTALLER=ON
            -DSMALL_BUILD_PACKAGE=ON
            -DSMALL_BUILD_WITH_MSVC_HACKS=ON
            -DSMALL_BUILD_WITH_UTF8=ON
            -DSMALL_BUILD_WITH_EXCEPTIONS=ON
            # Source dir
            ${small_SOURCE_DIR}
            # Build dir
            WORKING_DIRECTORY "${small_BINARY_DIR}"
            # Results
            COMMAND_ECHO STDOUT
            OUTPUT_VARIABLE small_CONFIGURE_OUTPUT
            ERROR_VARIABLE small_CONFIGURE_ERROR
            RESULT_VARIABLE small_CONFIGURE_RESULT
            RESULTS_VARIABLE small_CONFIGURE_RESULTS
            )
    if (small_CONFIGURE_RESULT EQUAL 0)
        set(small_CONFIGURE_OK ON)
    else ()
        message(FATAL_ERROR "small:configure: CMake execute_process command failed: \"${small_CONFIGURE_RESULT}\"")
        set(small_CONFIGURE_OK OFF)
    endif ()

    # Run build step
    execute_process(COMMAND "${CMAKE_COMMAND}" --build .
            WORKING_DIRECTORY "${small_BINARY_DIR}"
            )

    # Run install step
    execute_process(COMMAND "${CMAKE_COMMAND}" --install .
            WORKING_DIRECTORY "${small_BINARY_DIR}"
            )

    # Find package again
    set(ENV{small_ROOT} ${small_INSTALL_HINT})
    find_package(small REQUIRED CONFIG)
endif ()
version_requirement_message(small
        VERSION_FOUND ${small_VERSION}
        VERSION_LOCK ${small_VERSION_LOCK}
        VERSION_REQUIREMENTS ${small_VERSION_REQUIREMENT}
        PREFIX_HINT ${small_PREFIX_HINT})

#######################################################
### C++: Ranges                                     ###
#######################################################
# Set hints to local paths
set(range-v3_VERSION_LOCK 0.11.0)
set(range-v3_VERSION_REQUIREMENT ^0.11.0)
set_local_module_hints(range-v3 ${range-v3_VERSION_LOCK} ${range-v3_VERSION_REQUIREMENT})

# Look for lock version
if (NOT range-v3_FOUND)
    semver_split(${range-v3_VERSION_LOCK} range-v3_VERSION_LOCK)
    find_package(range-v3 ${range-v3_VERSION_LOCK_CORE} QUIET CONFIG)
endif ()

# Look for any version that matches our requirements
if (NOT range-v3_FOUND)
    find_package(range-v3 QUIET CONFIG)
    if (range-v3_FOUND AND range-v3_VERSION AND NOT DEFINED ENV{range-v3_ROOT})
        semver_requirements_compatible(${range-v3_VERSION} ${range-v3_VERSION_REQUIREMENT} ok)
        if (NOT ok)
            set(range-v3_FOUND FALSE)
        endif ()
    endif ()
endif ()

# Fetch range-v3 if we couldn't find a valid version
if (NOT range-v3_FOUND)
    message("Downloading range-v3...")
    FetchContent_Declare(range-v3
            URL https://github.com/ericniebler/range-v3/archive/refs/tags/0.11.0.zip
            SOURCE_DIR ${range-v3_SOURCE_HINT}
            BINARY_DIR ${range-v3_BINARY_HINT}
            )

    # Check if already populated
    FetchContent_GetProperties(range-v3)

    if (NOT range-v3_POPULATED)
        # Download files
        FetchContent_Populate(range-v3)

        # Run configure step
        execute_process(COMMAND "${CMAKE_COMMAND}"
                # CMake options
                -G "${CMAKE_GENERATOR}"
                -DCMAKE_CXX_COMPILER=${CMAKE_CXX_COMPILER}
                -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
                -DBUILD_SHARED_LIBS=${BUILD_SHARED_LIBS}
                # CMake install prefix
                -DCMAKE_INSTALL_PREFIX=${range-v3_INSTALL_HINT}
                # Package options
                -DRANGES_BUILD_CALENDAR_EXAMPLE=OFF
                -DRANGES_DEBUG_INFO=${DEBUG_MODE}
                -DRANGES_MODULES=OFF
                -DRANGES_NATIVE=OFF
                -DRANGES_LLVM_POLLY=OFF
                -DRANGES_PREFER_REAL_CONCEPTS=ON
                -DRANGES_DEEP_STL_INTEGRATION=OFF
                -DRANGE_V3_HEADER_CHECKS=OFF
                -DRANGES_VERBOSE_BUILD=OFF
                -DRANGE_V3_TESTS=OFF
                -DRANGE_V3_EXAMPLES=OFF
                -DRANGE_V3_PERF=OFF
                -DRANGE_V3_DOCS=OFF
                # Source dir
                ${range-v3_SOURCE_DIR}
                # Build dir
                WORKING_DIRECTORY "${range-v3_BINARY_DIR}"
                )

        # Run build step
        execute_process(COMMAND "${CMAKE_COMMAND}" --build .
                WORKING_DIRECTORY "${range-v3_BINARY_DIR}"
                )

        # Run install step
        execute_process(COMMAND "${CMAKE_COMMAND}" --install .
                WORKING_DIRECTORY "${range-v3_BINARY_DIR}"
                )

        # Find package again
        find_package(range-v3 REQUIRED)
    endif ()
endif ()
version_requirement_message(range-v3 VERSION_FOUND ${range-v3_VERSION} VERSION_LOCK ${range-v3_VERSION_LOCK} VERSION_REQUIREMENTS ${range-v3_VERSION_REQUIREMENT} PREFIX_HINT ${range-v3_PREFIX_HINT})

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
version_requirement_message(Asio
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

