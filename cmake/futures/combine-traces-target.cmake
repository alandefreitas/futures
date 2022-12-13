if (NOT FUTURES_TIME_TRACE OR NOT CLANG OR NOT CMAKE_CXX_COMPILER_VERSION VERSION_GREATER_EQUAL 9)
    return()
endif ()

#######################################################
### Find programs                                   ###
#######################################################
find_program(PYTHON_EXECUTABLE python3 python)
if (PYTHON_EXECUTABLE)
    message(STATUS "python found: ${PYTHON_EXECUTABLE}")
elseif (LINUX)
    message(FATAL_ERROR "python executable not found (try \"sudo apt-get -y install python3\")")
else ()
    message(FATAL_ERROR "python executable not found (https://www.python.org/downloads/windows/)")
endif ()

#######################################################
### Target                                          ###
#######################################################
set(COMBINE_SCRIPT ${FUTURES_ROOT_DIR}/dev-tools/combine_traces/combine_traces.py)
set(BUILD_DIR ${FUTURES_BINARY_ROOT_DIR})
set(TRACES_PATH ${FUTURES_BINARY_ROOT_DIR}/test/unit/CMakeFiles)
add_custom_target(
        combine-time-traces

        COMMAND "${CMAKE_COMMAND}"

        -D "PYTHON_COMMAND=${PYTHON_EXECUTABLE}"
        -D "PYTHON_SCRIPT=${COMBINE_SCRIPT}"
        -D "TRACES_PATH=${TRACES_PATH}"
        -D "BUILD_DIR=${BUILD_DIR}"
        -P "${PROJECT_SOURCE_DIR}/cmake/futures/combine-traces.cmake"

        WORKING_DIRECTORY "${FUTURES_BINARY_ROOT_DIR}"
        COMMENT "Combining time traces"
        VERBATIM
)
