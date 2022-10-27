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
foreach (FUTURE_FLAG FUTURES_PREFER_ASIO FUTURES_PREFER_BOOST)
    if (${FUTURE_FLAG})
        target_compile_definitions(headers INTERFACE ${FUTURE_FLAG})
    endif ()
endforeach ()

# Only link asio::asio if we FUTURES_PREFER_ASIO
# Otherwise, we use boost.asio or bundled asio
if (FUTURES_PREFER_ASIO AND (asio_FOUND OR Asio_FOUND))
    target_link_libraries(headers INTERFACE asio::asio)
    target_compile_definitions(headers INTERFACE FUTURES_HAS_ASIO)
    if (FUTURES_PREFER_ASIO OR NOT FUTURES_PREFER_BOOST)
        target_compile_definitions(headers INTERFACE FUTURES_PREFER_ASIO)
    endif ()
endif ()

# Link Boost headers whenever it's available because we need
# more than Boost.Asio.
if (Boost_FOUND)
    target_link_libraries(headers INTERFACE Boost::boost)
    target_compile_definitions(headers INTERFACE FUTURES_HAS_BOOST)
    if (FUTURES_PREFER_BOOST OR NOT (asio_FOUND OR Asio_FOUND))
        target_compile_definitions(headers INTERFACE FUTURES_PREFER_BOOST)
    endif ()
endif ()