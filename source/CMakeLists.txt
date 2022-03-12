###########################################################
### Target with separate compilation (not header-only)  ###
###########################################################
# This is a version of futures where ASIO is not header-only
add_library(futures-compiled futures-compiled/src.cpp)
target_link_libraries(futures-compiled PUBLIC futures)
target_compile_definitions(futures-compiled PUBLIC FUTURES_SEPARATE_COMPILATION)
add_library(futures::futures-compiled ALIAS futures-compiled)