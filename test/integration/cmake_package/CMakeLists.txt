#######################################################
### Futures                                         ###
#######################################################
cmake_minimum_required(VERSION 3.10)
project(application)

#[find_package
find_package(futures REQUIRED)
#]

if (CMAKE_VERSION VERSION_GREATER_EQUAL 3.11.4)
    #[find_or_fetch
    include(FetchContent)
    find_package(futures)
    if (NOT futures_FOUND)
        FetchContent_Declare(futures
                GIT_REPOSITORY https://github.com/alandefreitas/futures
                GIT_TAG origin/master # or whatever tag you want
                )
        FetchContent_MakeAvailable(futures)
    endif ()
    #]
endif ()

#[link
add_executable(application main.cpp)
target_link_libraries(application PRIVATE futures::futures)
#]
