if (NOT FUTURES_CATCH2_REPORTER STREQUAL junit)
    return()
endif ()

#######################################################
### Find programs                                   ###
#######################################################
find_program(PYTHON_EXECUTABLE python3 python)
if (PYTHON_EXECUTABLE)
    message(STATUS "python found: ${PYTHON_EXECUTABLE}")
elseif (UNIX)
    message(FATAL_ERROR "python executable not found (try \"sudo apt-get -y install python3\")")
else ()
    message(FATAL_ERROR "python executable not found (https://www.python.org/downloads/windows/)")
endif ()

#######################################################
### Target                                          ###
#######################################################
set(COMBINE_SCRIPT ${FUTURES_ROOT_DIR}/dev-tools/junit_format/junit_format.py)
set(BUILD_DIR ${FUTURES_BINARY_ROOT_DIR})
set(REPORTS_PATH ${FUTURES_BINARY_ROOT_DIR}/test/unit/reports)
add_custom_target(
        junit-format

        COMMAND "${CMAKE_COMMAND}"

        -D "PYTHON_COMMAND=${PYTHON_EXECUTABLE}"
        -D "PYTHON_SCRIPT=${COMBINE_SCRIPT}"
        -D "REPORTS_PATH=${REPORTS_PATH}"
        -D "BUILD_DIR=${BUILD_DIR}"
        -P "${PROJECT_SOURCE_DIR}/cmake/futures/junit-format.cmake"

        WORKING_DIRECTORY "${FUTURES_BINARY_ROOT_DIR}"
        COMMENT "Formatting junit reports"
        VERBATIM
)
