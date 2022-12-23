cmake_minimum_required(VERSION 3.10)

macro(default name)
    if (NOT DEFINED "${name}")
        set("${name}" "${ARGN}")
    endif ()
    message(STATUS "${name}=${${name}}")
endmacro()

default(PYTHON_COMMAND python3)
default(PYTHON_SCRIPT ${CMAKE_CURRENT_LIST_DIR}/../../dev-tools/combine_traces/combine_traces.py)
default(REPORTS_PATH ${CMAKE_CURRENT_LIST_DIR}/../../build/test/unit/reports)
default(BUILD_DIR ${CMAKE_CURRENT_LIST_DIR}/../../build)

file(GLOB_RECURSE REPORT_FILES "${REPORTS_PATH}/*.junit")
execute_process(
        COMMAND ${PYTHON_COMMAND} ${PYTHON_SCRIPT} ${REPORT_FILES}
        WORKING_DIRECTORY ${BUILD_DIR}
        RESULT_VARIABLE result
        OUTPUT_VARIABLE output
)
if (NOT result EQUAL "0")
    message(FATAL_ERROR "'junit-format.cmake': script returned with \"${result}\".\nOutput: ${output}")
else()
    message(STATUS "'junit-format.cmake': success. script returned with \"${result}\".\nOutput: ${output}")
endif ()
