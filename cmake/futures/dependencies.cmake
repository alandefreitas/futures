#######################################################
### Dependencies                                    ###
#######################################################
find_package(Threads)

# There's no fetch contents for Boost.
find_package(Boost CONFIG)

# Standalone Asio does not provide a FindAsio.cmake or ConfigAsio.cmake script,
# so futures provide its own FindAsio.cmake script.
# If Asio is not found, as it's usually not, futures will also install the library
# and the FindAsio.cmake script in such a way that a futures installation can later
# find it.
find_package(Asio 1.21.0 QUIET)
if (NOT Asio_FOUND)
    # Download Asio
    FetchContent_Declare(Asio
            GIT_REPOSITORY https://github.com/chriskohlhoff/asio.git
            GIT_TAG        asio-1-21-0
            )
    FetchContent_GetProperties(Asio)
    if (NOT Asio_POPULATED)
        FetchContent_Populate(Asio)
    endif ()

    # Create imported CMake library
    add_library(asio::asio INTERFACE IMPORTED)
    set_target_properties(asio::asio PROPERTIES INTERFACE_INCLUDE_DIRECTORIES "${asio_SOURCE_DIR}/asio/include")

    # Set the flag on to indicate we should link this library
    set(Asio_FOUND ON)
endif ()
