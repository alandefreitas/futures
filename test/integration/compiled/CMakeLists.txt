#######################################################
### Futures                                         ###
#######################################################
cmake_minimum_required(VERSION 3.10)
project(application)

set(FUTURES_SRC ${CMAKE_CURRENT_SOURCE_DIR}/../../..)

#[include_dirs
add_executable(application main.cpp futures-src.cpp)
target_include_directories(application PRIVATE ${FUTURES_SRC}/include)
target_compile_definitions(application PRIVATE FUTURES_SEPARATE_COMPILATION)
target_compile_features(application PRIVATE cxx_std_14)

find_package(Threads)
target_link_libraries(application PRIVATE ${CMAKE_THREAD_LIBS_INIT})
#]