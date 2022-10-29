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

# Compiled futures also uses compiled Asio
if ((asio_FOUND OR Asio_FOUND) AND (NOT Boost_FOUND OR NOT FUTURES_PREFER_BOOST))
    target_compile_definitions(futures PUBLIC ASIO_SEPARATE_COMPILATION)
elseif (Boost_FOUND)
    target_compile_definitions(futures PUBLIC BOOST_ASIO_SEPARATE_COMPILATION)
else()
    target_compile_definitions(futures PUBLIC ASIO_SEPARATE_COMPILATION)
endif()
