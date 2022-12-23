#######################################################
### Dependencies                                    ###
#######################################################
find_package(Threads)

if (FUTURES_USE_FIND_PACKAGE)
    # There's no fetch contents for Boost.
    find_package(Boost CONFIG)

    # Standalone Asio does not provide a FindAsio.cmake or ConfigAsio.cmake script,
    # so futures provide its own FindAsio.cmake script.
    # If Asio is not found, as it's usually not, and then we fetch it, futures will also
    # install the library and the FindAsio.cmake script in such a way that the futures
    # installation can later find it.
    find_package(Asio 1.21.0)
endif ()

if (FUTURES_USE_FETCH_CONTENT AND CMAKE_VERSION VERSION_GREATER_EQUAL 3.11.4)
    include(FetchContent)

    if (NOT Asio_FOUND)
        message(STATUS "Fetching Asio")

        # Download Asio
        FetchContent_Declare(Asio
                GIT_REPOSITORY https://github.com/chriskohlhoff/asio.git
                GIT_TAG asio-1-21-0
                )
        FetchContent_GetProperties(Asio)
        if (NOT Asio_POPULATED)
            FetchContent_Populate(Asio)
        endif ()

        # Create imported CMake library
        add_library(asio INTERFACE)
        target_include_directories(asio INTERFACE
                $<BUILD_INTERFACE:${asio_SOURCE_DIR}/asio/include>
                $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)
        add_library(asio::asio ALIAS asio)

        set(Asio_FOUND ON)
        set(Asio_FETCHED ON)
        message(STATUS "asio fetched: ${asio_SOURCE_DIR}")

        # If asio is not imported, it goes to the install list
        install(TARGETS asio
                EXPORT futures-targets
                LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
                ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
                )
    endif ()
endif ()
