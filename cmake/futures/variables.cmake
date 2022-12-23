#######################################################
### Environment                                     ###
#######################################################
include(cmake/functions/all.cmake)
set_master_project_booleans()
set_crosscompiling_booleans()
set_debug_booleans()
set_optimization_flags()
set_compiler_booleans()

#######################################################
### Installer                                       ###
#######################################################
# CMake dependencies for installer
include(CMakePackageConfigHelpers)
include(GNUInstallDirs)

#######################################################
### Enable Find*.cmake scripts                      ###
#######################################################
# Append ./cmake directory to our include paths for the find_package scripts
list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

#######################################################
### Disable warnings in FetchContent                ###
#######################################################
# target_include_directories with the SYSTEM modifier will request the compiler
# to omit warnings from the provided paths, if the compiler supports that
# This is to provide a user experience similar to find_package when
# add_subdirectory or FetchContent is used to consume this project
set(warning_guard "")
if (NOT MASTER_PROJECT)
    option(
            FUTURES_INCLUDES_WITH_SYSTEM
            "Use SYSTEM modifier for futures' includes, disabling warnings"
            ON
    )
    mark_as_advanced(FUTURES_INCLUDES_WITH_SYSTEM)
    if (FUTURES_INCLUDES_WITH_SYSTEM)
        set(warning_guard SYSTEM)
    endif ()
endif ()
