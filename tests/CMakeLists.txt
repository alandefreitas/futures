if (CMAKE_CROSSCOMPILING)
    return()
endif ()

add_subdirectory(unit)
add_subdirectory(stress)