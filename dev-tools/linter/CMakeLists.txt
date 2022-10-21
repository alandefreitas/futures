###########################################################
### Linter application                                  ###
###########################################################
# This is a linter with a few rules that go beyond clang-format
add_executable(linter
        application.hpp
        application.cpp
        config.hpp
        config.cpp
        linter.cpp
        filesystem.hpp
        filesystem.cpp
)
target_compile_features(linter PUBLIC cxx_std_17)

###########################################################
### Lint library files                                  ###
###########################################################
set(DEP_INCLUDE_DIRS)
if (Boost_FOUND)
    set(DEP_INCLUDE_DIRS ${Boost_INCLUDE_DIRS})
endif()
if (Asio_FOUND AND asio_SOURCE_DIR)
    set(DEP_INCLUDE_DIRS ${DEP_INCLUDE_DIRS} ${asio_SOURCE_DIR}/asio/include)
endif()

# Generate a single-header version of futures
add_custom_target(
        lint_futures
        COMMAND linter
        # Source include dirs
        --include_paths "${FUTURES_ROOT_DIR}/include"
        # Dependency include dirs, so we can copy files from there
        --dep_include_paths ${DEP_INCLUDE_DIRS}
        # Directory with includes for our bundled dependencies
        --deps_headers_path "${FUTURES_ROOT_DIR}/include/futures/detail/deps"
        # Directory for a our bundled dependencies
        --bundled_deps_path "${FUTURES_ROOT_DIR}/include/futures/detail/bundled"
        # Directory prefixes we sould ignore when bundling external headers
        --bundle_ignore_prefix "arpa" "linux" "net" "netinet" "windows.h" "android" "winerror" "netdb" "valgrind"
        # Linter options
        --show_progress
        # CMake command options
        DEPENDS linter ${FUTURES_HEADERS}
        COMMENT "Run linter"
        VERBATIM
)
if (FUTURES_ALWAYS_LINT)
    add_dependencies(futures lint_futures)
endif()
