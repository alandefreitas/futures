#######################################################
### Dependencies                                    ###
#######################################################
find_package(Threads)

find_package(Boost CONFIG)

find_package(Asio 1.21.0 QUIET)
if (NOT Asio_FOUND)
    FetchContent_Declare(Asio
            GIT_REPOSITORY https://github.com/chriskohlhoff/asio.git
            GIT_TAG        asio-1-21-0
            )
    FetchContent_GetProperties(Asio)
    if (NOT Asio_POPULATED)
        FetchContent_Populate(Asio)
    endif ()
    add_library(asio::asio INTERFACE IMPORTED)
    message("Include asio from ${asio_SOURCE_DIR}/asio/include")
    set_target_properties(asio::asio PROPERTIES INTERFACE_INCLUDE_DIRECTORIES "${asio_SOURCE_DIR}/asio/include")
    set(Asio_FOUND ON)
endif ()
