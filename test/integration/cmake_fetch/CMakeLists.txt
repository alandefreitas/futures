#######################################################
### Futures                                         ###
#######################################################
cmake_minimum_required(VERSION 3.10)
project(application)

if (CMAKE_VERSION VERSION_LESS 3.11.4)
    return()
endif ()

include(FetchContent)
FetchContent_Declare(futures
        GIT_REPOSITORY https://github.com/alandefreitas/futures
        GIT_TAG origin/develop # or whatever tag you want
        )
FetchContent_MakeAvailable(futures)

if (FALSE)
    #[fetchcontent
    include(FetchContent)
    FetchContent_Declare(futures
            GIT_REPOSITORY https://github.com/alandefreitas/futures
            GIT_TAG origin/master # or whatever tag you want
            )
    FetchContent_MakeAvailable(futures)
    #]
endif ()

#[link
add_executable(application main.cpp)
target_link_libraries(application PRIVATE futures::futures)
#]
