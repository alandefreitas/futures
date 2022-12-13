#######################################################
### Find programs                                   ###
#######################################################
find_program(CLANG_FORMAT_EXECUTABLE clang-format)
if (CLANG_FORMAT_EXECUTABLE)
    message(STATUS "clang-format found: ${CLANG_FORMAT_EXECUTABLE}")
else ()
    message(FATAL_ERROR "clang-format executable not found (https://clang.llvm.org/docs/ClangFormat.html)")
endif()

find_program(CODESPELL_EXECUTABLE codespell)
if (CODESPELL_EXECUTABLE)
    message(STATUS "codespell found: ${CODESPELL_EXECUTABLE}")
else ()
    message(FATAL_ERROR "codespell executable not found (https://github.com/codespell-project/codespell)")
endif()

#######################################################
### Clang-format                                    ###
#######################################################
set(
        FORMAT_PATTERNS
        include/*.hpp include/*.cpp include/*.ipp
        src/*.hpp src/*.cpp src/*.ipp
        test/*.cpp test/*.hpp test/*.ipp
)

add_custom_target(
        format-check

        COMMAND "${CMAKE_COMMAND}"

        -D "FORMAT_COMMAND=${CLANG_FORMAT_EXECUTABLE}"
        -D "PATTERNS=${FORMAT_PATTERNS}"
        -P "${PROJECT_SOURCE_DIR}/cmake/futures/lint.cmake"

        WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}"
        COMMENT "Linting the code"
        VERBATIM
)

add_custom_target(
        format-fix

        COMMAND "${CMAKE_COMMAND}"

        -D "FORMAT_COMMAND=${CLANG_FORMAT_EXECUTABLE}"
        -D "PATTERNS=${FORMAT_PATTERNS}"
        -D FIX=YES
        -P "${PROJECT_SOURCE_DIR}/cmake/futures/lint.cmake"

        WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}"
        COMMENT "Fixing the code"
        VERBATIM
)

#######################################################
### Clang-format                                    ###
#######################################################
find_program(CODESPELL_EXECUTABLE codespell)
if (CODESPELL_EXECUTABLE)
    message(STATUS "codespell found: ${CODESPELL_EXECUTABLE}")
else ()
    message(FATAL_ERROR "codespell executable not found (https://github.com/linux-test-project/lcov)")
endif()

add_custom_target(
        spell-check

        COMMAND "${CMAKE_COMMAND}"
        -D "SPELL_COMMAND=${CODESPELL_EXECUTABLE}"
        -P "${PROJECT_SOURCE_DIR}/cmake/futures/spell.cmake"

        WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}/include"
        COMMENT "Checking spelling"
        VERBATIM
)

add_custom_target(
        spell-fix

        COMMAND "${CMAKE_COMMAND}"
        -D "SPELL_COMMAND=${CODESPELL_EXECUTABLE}"
        -D FIX=YES
        -P "${PROJECT_SOURCE_DIR}/cmake/futures/spell.cmake"

        WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}/include"
        COMMENT "Fixing spelling errors"
        VERBATIM
)
