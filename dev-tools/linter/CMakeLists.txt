###########################################################
### Linter application                                  ###
###########################################################
# This is a linter with a few rules that go beyond clang-format
add_executable(linter main.cpp)
target_compile_features(linter PUBLIC cxx_std_17)

###########################################################
### Lint library files                                  ###
###########################################################
# Generate a single-header version of futures
add_custom_target(
        lint_futures
        COMMAND linter
        # Files
        --include_paths "${FUTURES_ROOT_DIR}/include"
        # Linter options
        --show_progress
        # CMake command options
        DEPENDS linter ${FUTURES_HEADERS}
        COMMENT "Run linter"
        VERBATIM
)
add_dependencies(futures lint_futures)
