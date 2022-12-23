#######################################################
### Futures                                         ###
#######################################################
cmake_minimum_required(VERSION 3.10)
project(application)

set(FUTURES_SRC ${CMAKE_CURRENT_SOURCE_DIR}/../../..)

#[add_subdir
add_subdirectory(${FUTURES_SRC} ${CMAKE_CURRENT_BINARY_DIR}/_deps/futures)
add_executable(application main.cpp)
target_link_libraries(application PRIVATE futures::futures)
#]
