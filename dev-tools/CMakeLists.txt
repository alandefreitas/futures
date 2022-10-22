if (CMAKE_CROSSCOMPILING)
    return()
endif()

if (FUTURES_BUILD_LINTER)
    add_subdirectory(linter)
endif()