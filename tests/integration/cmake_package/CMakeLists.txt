#######################################################
### Futures                                         ###
#######################################################
cmake_minimum_required(VERSION 3.16)
project(application)

#[find_package
find_package(futures REQUIRED)
#]

#[find_or_fetch
include(FetchContent)
find_package(futures)
if (NOT futures_FOUND)
    FetchContent_Declare(futures
            GIT_REPOSITORY https://github.com/alandefreitas/futures
            GIT_TAG origin/master # or whatever tag you want
            )
    FetchContent_MakeAvailable(futures)
endif()
#]

#[link
add_executable(application main.cpp)
target_link_libraries(application PRIVATE futures::futures)
#]
