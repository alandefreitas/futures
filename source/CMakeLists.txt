###########################################################
### Target with separate compilation (not header-only)  ###
###########################################################
# This version of futures is not header-only by default
add_library(futures futures/src.cpp)
add_library(futures::futures ALIAS futures)
target_link_libraries(futures PUBLIC headers)
target_compile_definitions(futures PUBLIC FUTURES_SEPARATE_COMPILATION)

if (asio_FOUND OR Asio_FOUND)
    add_library(futures_asio futures/src.cpp)
    add_library(futures::futures_asio ALIAS futures_asio)
    target_link_libraries(futures_asio PUBLIC headers_asio)
    target_compile_definitions(futures_asio PUBLIC FUTURES_SEPARATE_COMPILATION)
endif()

if (Boost_FOUND)
    add_library(futures_boost futures/src.cpp)
    add_library(futures::futures_boost ALIAS futures_boost)
    target_link_libraries(futures_boost PUBLIC headers_boost)
    target_compile_definitions(futures_boost PUBLIC FUTURES_SEPARATE_COMPILATION)
endif ()
