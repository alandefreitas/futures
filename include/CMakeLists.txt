#######################################################
### Futures library                                 ###
#######################################################
# The target futures::headers works as a header only library
# CONFIGURE_DEPENDS works on most generators now
file(GLOB_RECURSE FUTURES_HEADERS CONFIGURE_DEPENDS futures/**/*.hpp)
add_library(headers INTERFACE)
add_library(futures::headers ALIAS headers)

macro(set_futures_options TARGET_NAME)
    # This library requires C++17
    target_compile_features(${TARGET_NAME} INTERFACE cxx_std_17)

    # Include directories (development and installation)
    target_include_directories(${TARGET_NAME}
            INTERFACE $<BUILD_INTERFACE:${FUTURES_ROOT_DIR}/include>
            $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
            )

    if (FUTURES_BUILD_WITH_EXCEPTIONS AND MSVC)
        target_compile_options(${TARGET_NAME} INTERFACE /EHsc)
    endif ()

    if (FUTURES_BUILD_WITH_PEDANTIC_WARNINGS)
        if (MSVC)
            target_compile_options(${TARGET_NAME} INTERFACE /W3 /WX)
        else ()
            target_compile_options(${TARGET_NAME} INTERFACE -Wall -Wextra -pedantic -Werror -ftemplate-backtrace-limit=0)
        endif ()
    endif ()

    target_link_libraries(${TARGET_NAME} INTERFACE ${CMAKE_THREAD_LIBS_INIT})
endmacro()

set_futures_options(headers)

#######################################################
### Link Dependencies                               ###
#######################################################
if (FUTURES_PREFER_STANDALONE_DEPENDENCIES AND (asio_FOUND OR Asio_FOUND))
    target_link_libraries(headers INTERFACE asio::asio)
    target_compile_definitions(headers INTERFACE FUTURES_PREFER_STANDALONE_DEPENDENCIES)
elseif (FUTURES_PREFER_BOOST_DEPENDENCIES AND Boost_FOUND)
    target_link_libraries(headers INTERFACE Boost::boost)
    target_compile_definitions(headers INTERFACE FUTURES_PREFER_BOOST_DEPENDENCIES)
elseif(asio_FOUND OR Asio_FOUND)
    target_link_libraries(headers INTERFACE asio::asio)
    target_compile_definitions(headers INTERFACE FUTURES_PREFER_STANDALONE_DEPENDENCIES)
elseif(Boost_FOUND)
    target_link_libraries(headers INTERFACE Boost::boost)
    target_compile_definitions(headers INTERFACE FUTURES_PREFER_BOOST_DEPENDENCIES)
else()
    message(FATAL_ERROR "Neither Boost nor Asio are available")
endif ()

#######################################################
### Targets for each dependency found               ###
#######################################################
if (asio_FOUND OR Asio_FOUND)
    add_library(headers_asio INTERFACE)
    add_library(futures::headers_asio ALIAS headers_asio)
    set_futures_options(headers_asio)
    target_link_libraries(headers_asio INTERFACE asio::asio)
    target_compile_definitions(headers_asio INTERFACE FUTURES_PREFER_STANDALONE_DEPENDENCIES)
endif ()

if (Boost_FOUND)
    add_library(headers_boost INTERFACE)
    add_library(futures::headers_boost ALIAS headers_boost)
    set_futures_options(headers_boost)
    target_link_libraries(headers_boost INTERFACE Boost::boost)
    target_compile_definitions(headers_boost INTERFACE FUTURES_PREFER_BOOST_DEPENDENCIES)
endif ()
