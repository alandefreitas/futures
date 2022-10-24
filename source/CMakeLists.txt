###########################################################
### Target with separate compilation (not header-only)  ###
###########################################################
# This version of futures is not header-only by default
# This is the library we use in the general case, even
# though futures::headers is still always available
add_library(futures futures/src.cpp)
add_library(futures::futures ALIAS futures)
target_link_libraries(futures PUBLIC headers)
target_compile_definitions(futures PUBLIC FUTURES_SEPARATE_COMPILATION)