#######################################################
### Futures library                                 ###
#######################################################
# The target futures::headers works as a header only library
# CONFIGURE_DEPENDS works on most generators now
file(GLOB_RECURSE FUTURES_HEADERS CONFIGURE_DEPENDS futures/**/*.hpp)
add_library(headers INTERFACE)
add_library(futures::headers ALIAS headers)

# This library requires C++17
target_compile_features(headers INTERFACE cxx_std_17)

# Include directories (development and installation)
target_include_directories(headers
        INTERFACE $<BUILD_INTERFACE:${FUTURES_ROOT_DIR}/include>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
        )

if (FUTURES_BUILD_WITH_EXCEPTIONS AND MSVC)
    # MSVC needs /EHsc to enable exceptions
    target_compile_options(headers INTERFACE /EHsc)
endif ()

if (FUTURES_PEDANTIC_WARNINGS)
    if (MSVC)
        target_compile_options(headers INTERFACE /W3)
    else ()
        target_compile_options(headers INTERFACE -Wall -Wextra -pedantic -ftemplate-backtrace-limit=0)
    endif ()
endif ()
if (FUTURES_WARNINGS_AS_ERRORS)
    if (MSVC)
        target_compile_options(headers INTERFACE /WX)
    else ()
        target_compile_options(headers INTERFACE -Werror)
    endif ()
endif ()

if (CLANG)
    # Disable allocator::rebind<void> warning on external deps
    # futures/detail/bundled/boost/intrusive/pointer_rebind.hpp
    target_compile_options(
            headers
            INTERFACE
            -Wno-error=deprecated-declarations)
endif ()

# Threads
target_link_libraries(headers INTERFACE ${CMAKE_THREAD_LIBS_INIT})

#######################################################
### Link Dependencies                               ###
#######################################################
foreach (FUTURE_FLAG FUTURES_PREFER_STANDALONE_ASIO FUTURES_PREFER_BOOST)
    if (${FUTURE_FLAG})
        target_compile_definitions(headers INTERFACE ${FUTURE_FLAG})
    endif ()
endforeach ()

# Link appropriate version of asio::asio
if ((asio_FOUND OR Asio_FOUND) AND (NOT Boost_FOUND OR NOT FUTURES_PREFER_BOOST))
    target_link_libraries(headers INTERFACE asio::asio)
    target_compile_definitions(headers INTERFACE FUTURES_HAS_STANDALONE_ASIO)
    target_compile_definitions(headers INTERFACE FUTURES_USE_STANDALONE_ASIO)
    target_compile_definitions(headers INTERFACE FUTURES_PREFER_STANDALONE_ASIO)
elseif (Boost_FOUND)
    target_link_libraries(headers INTERFACE Boost::boost)
    target_compile_definitions(headers INTERFACE FUTURES_HAS_BOOST)
    target_compile_definitions(headers INTERFACE FUTURES_USE_BOOST)
    target_compile_definitions(headers INTERFACE FUTURES_PREFER_BOOST)
else ()
    target_compile_definitions(headers INTERFACE FUTURES_HAS_BUNDLED_ASIO)
    target_compile_definitions(headers INTERFACE FUTURES_USE_BUNDLED_ASIO)
    target_compile_definitions(headers INTERFACE FUTURES_PREFER_BUNDLED_ASIO)
endif ()