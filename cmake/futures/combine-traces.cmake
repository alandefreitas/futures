cmake_minimum_required(VERSION 3.14)

macro(default name)
    if(NOT DEFINED "${name}")
        set("${name}" "${ARGN}")
    endif()
    message(STATUS "${name}=${${name}}")
endmacro()

default(PYTHON_COMMAND python3)
default(PYTHON_SCRIPT ${CMAKE_CURRENT_LIST_DIR}/../../dev-tools/combine_traces/combine_traces.py)
default(TRACES_PATH ${CMAKE_CURRENT_LIST_DIR}/../../build)
default(BUILD_DIR ${CMAKE_CURRENT_LIST_DIR}/../../build)

file(GLOB_RECURSE TRACE_FILES "${TRACES_PATH}/**/*.cpp.json")
message(STATUS "TRACE_FILES=${TRACE_FILES}")
execute_process(
        COMMAND ${PYTHON_COMMAND} ${PYTHON_SCRIPT} ${TRACE_FILES}
        WORKING_DIRECTORY ${BUILD_DIR}
        RESULT_VARIABLE result
        OUTPUT_VARIABLE output
)
if(NOT result EQUAL "0")
    message(FATAL_ERROR "'combine-traces.cmake': script returned with \"${result}\".\nOutput: ${output}")
endif()
