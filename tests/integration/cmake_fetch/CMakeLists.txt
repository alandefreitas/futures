#######################################################
### Futures                                         ###
#######################################################
cmake_minimum_required(VERSION 3.16)
project(application)

#[fetchcontent
include(FetchContent)
FetchContent_Declare(futures
        GIT_REPOSITORY https://github.com/alandefreitas/futures
        GIT_TAG origin/master # or whatever tag you want
        )
FetchContent_MakeAvailable(futures)
#]

#[link
add_executable(application main.cpp)
target_link_libraries(application PRIVATE futures::futures)
#]
