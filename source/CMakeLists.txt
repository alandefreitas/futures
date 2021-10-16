#######################################################
### Futures library                                 ###
#######################################################
add_library(futures
        # ASIO not header-only
        futures/detail/asio-src.cpp
        # Traits
        futures/detail/traits/future_return.h
        futures/detail/traits/is_callable.h
        futures/detail/traits/is_executor_then_continuation.h
        futures/detail/traits/is_executor_then_function.h
        futures/detail/traits/is_future.h
        futures/detail/traits/is_future_continuation.h
        futures/detail/traits/is_reference_wrapper.h
        futures/detail/traits/is_tuple.h
        futures/detail/traits/to_future.h
        futures/detail/traits/unwrap_future.h
        # Better default executors for async
        futures/detail/default_executor.h
        futures/detail/default_executor.cpp
        futures/detail/inline_executor.h
        futures/detail/inline_executor.cpp
        # Future types
        futures/detail/stop_token.h
        futures/detail/basic_future.h
        futures/detail/tuple_algorithm.h
        futures/detail/when_all.h
        futures/detail/when_any.h
        futures/detail/when_any_result.h
        # Utilities
        futures/detail/then.h
        futures/detail/ready_future.h
        # Async algorithms
        futures/algorithm/algorithm_traits.h
        futures/algorithm/all_of.h
        futures/algorithm/any_of.h
        futures/algorithm/count.h
        futures/algorithm/count_if.h
        futures/algorithm/find.h
        futures/algorithm/find_if.h
        futures/algorithm/find_if_not.h
        futures/algorithm/for_each.h
        futures/algorithm/none_of.h
        futures/algorithm/partitioner.h
        futures/algorithm/reduce.h
        # Run async functions
        futures/algorithm.h
        futures/futures.h
        )

# Target aliases
# This emulates find_package when add_subdirectory has been used
add_library(futures::futures ALIAS futures)

# This library requires C++17
target_compile_features(futures PUBLIC cxx_std_17)

# Include directories (development and installation)
target_include_directories(futures
        INTERFACE $<BUILD_INTERFACE:${FUTURES_ROOT_DIR}/source>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
        )

# Compiler options
# MSVC compatibility
target_bigobj_options(futures)
target_utf8_options(futures)
target_nominmax_definition(futures)

if (BUILD_WITH_EXCEPTIONS)
    target_exception_options(futures)
endif ()

# Always enable warnings in development mode
maybe_target_pedantic_warnings(futures)

#######################################################
### Dependencies                                    ###
#######################################################
target_link_libraries(futures PUBLIC small::small)
target_link_libraries(futures PUBLIC asio::asio)
target_compile_definitions(futures PUBLIC ASIO_SEPARATE_COMPILATION)
target_link_libraries(futures PUBLIC range-v3::range-v3)
target_link_libraries(futures PUBLIC ${CMAKE_THREAD_LIBS_INIT})

#######################################################
### Installer                                       ###
#######################################################
if (FUTURES_BUILD_INSTALLER)
    # Install targets
    install(TARGETS futures
            EXPORT futures-targets
            LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
            ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
            )

    # Install headers
    install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/futures
            DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
            FILES_MATCHING PATTERN "*.h"
            )

    # Install cmake script
    install(EXPORT futures-targets
            FILE futures-targets.cmake
            NAMESPACE futures::
            DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/futures
            )
endif ()
