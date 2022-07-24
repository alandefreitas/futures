#######################################################
### Find programs                                   ###
#######################################################
find_program(LCOV_EXECUTABLE lcov)
if (LCOV_EXECUTABLE)
    message(STATUS "lcov found: ${LCOV_EXECUTABLE}")
elseif (LINUX)
    message(FATAL_ERROR "lcov executable not found (try \"sudo apt-get -y install lcov\")")
else()
    message(FATAL_ERROR "lcov executable not found (https://github.com/linux-test-project/lcov)")
endif()

find_program(GENHTML_EXECUTABLE genhtml)
if (GENHTML_EXECUTABLE)
    message(STATUS "genhtml found: ${GENHTML_EXECUTABLE}")
else ()
    message(FATAL_ERROR "genhtml executable not found (https://github.com/linux-test-project/lcov)")
endif()

#######################################################
### Commands                                        ###
#######################################################
set(
        COVERAGE_TRACE_COMMAND

        "${LCOV_EXECUTABLE}"
        --capture # capture coverage data
        --directory "${PROJECT_BINARY_DIR}" #  Use .da files in DIR instead of kernel
        --output-file "${PROJECT_BINARY_DIR}/coverage.info" #  Write data to FILENAME instead of stdout

        --include "${PROJECT_SOURCE_DIR}/include/futures/*" # Include files
)

set(
        COVERAGE_HTML_COMMAND

        "${GENHTML_EXECUTABLE}"
        --legend # Include color legend in HTML output
        --frames # Use HTML frames for source code view
        # --quiet # Do not print progress messages
        "${PROJECT_BINARY_DIR}/coverage.info" # info file

        --prefix "${PROJECT_SOURCE_DIR}/include" # Remove PREFIX from all directory names
        --output-directory "${PROJECT_BINARY_DIR}/coverage_html" # Write HTML output to OUTDIR
)

#######################################################
### Target                                          ###
#######################################################
add_custom_target(
        coverage

        COMMAND ${COVERAGE_TRACE_COMMAND}
        COMMAND ${COVERAGE_HTML_COMMAND}

        COMMENT "Generating coverage report"

        VERBATIM
)
message(STATUS "${COVERAGE_TRACE_COMMAND}")
message(STATUS "${COVERAGE_HTML_COMMAND}")
