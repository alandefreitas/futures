if (FUTURES_BUILD_LINTER)
    add_subdirectory(linter)
endif()

if (FUTURES_BUILD_SINGLE_HEADER)
    add_subdirectory(amalgamator)
endif()

