#######################################################
### Futures library                                 ###
#######################################################
add_library(futures
        futures/adaptor/detail/continuation_unwrap.h
        futures/adaptor/detail/traits/has_get.h
        futures/adaptor/detail/traits/is_callable.h
        futures/adaptor/detail/traits/is_executor_then_continuation.h
        futures/adaptor/detail/traits/is_reference_wrapper.h
        futures/adaptor/detail/traits/is_single_type_tuple.h
        futures/adaptor/detail/traits/is_tuple.h
        futures/adaptor/detail/traits/is_tuple_invocable.h
        futures/adaptor/detail/traits/is_when_any_result.h
        futures/adaptor/detail/traits/tuple_type_all_of.h
        futures/adaptor/detail/traits/tuple_type_concat.h
        futures/adaptor/detail/traits/tuple_type_transform.h
        futures/adaptor/detail/traits/type_member_or.h
        futures/adaptor/detail/tuple_algorithm.h
        futures/adaptor/ready_future.h
        futures/adaptor/then.h
        futures/adaptor/when_all.h
        futures/adaptor/when_any.h
        futures/adaptor/when_any_result.h
        futures/algorithm.h
        futures/algorithm/algorithm_traits.h
        futures/algorithm/all_of.h
        futures/algorithm/any_of.h
        futures/algorithm/count.h
        futures/algorithm/count_if.h
        futures/algorithm/detail/try_async.h
        futures/algorithm/find.h
        futures/algorithm/find_if.h
        futures/algorithm/find_if_not.h
        futures/algorithm/for_each.h
        futures/algorithm/none_of.h
        futures/algorithm/partitioner.h
        futures/algorithm/reduce.h
        futures/config/asio-compile.h
        futures/config/asio_include.h
        futures/config/small_vector_include.h
        futures/executor/default_executor.h
        futures/executor/inline_executor.h
        futures/futures.h
        futures/futures/async.h
        futures/futures/await.h
        futures/futures/basic_future.h
        futures/futures/detail/continuations_source.h
        futures/futures/detail/intrusive_ptr.h
        futures/futures/detail/shared_state.h
        futures/futures/detail/task.h
        futures/futures/detail/throw_exception.h
        futures/futures/detail/to_address.h
        futures/futures/detail/traits/async_result_of.h
        futures/futures/detail/traits/async_result_value_type.h
        futures/futures/detail/traits/has_is_ready.h
        futures/futures/detail/traits/type_member_or_void.h
        futures/futures/future_error.h
        futures/futures/launch.h
        futures/futures/packaged_task.h
        futures/futures/promise.h
        futures/futures/stop_token.h
        futures/futures/traits/future_return.h
        futures/futures/traits/is_executor_then_function.h
        futures/futures/traits/is_future.h
        futures/futures/traits/is_future_continuation.h
        futures/futures/traits/to_future.h
        futures/futures/traits/unwrap_future.h

        futures/executor/default_executor.cpp
        futures/executor/inline_executor.cpp
        )

# Target aliases
# This emulates find_package when add_subdirectory has been used
add_library(futures::futures ALIAS futures)

# This library requires C++17
target_compile_features(futures PUBLIC cxx_std_17)

# Include directories (development and installation)
target_include_directories(futures
        PUBLIC $<BUILD_INTERFACE:${FUTURES_ROOT_DIR}/source>
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
if (Boost_FOUND)
    target_link_libraries(futures PUBLIC Boost::boost)
endif()
if (small_FOUND OR Small_FOUND)
    target_link_libraries(futures PUBLIC small::small)
endif()
if (asio_FOUND OR Asio_FOUND)
    target_link_libraries(futures PUBLIC asio::asio)
endif()
target_link_libraries(futures PUBLIC ${CMAKE_THREAD_LIBS_INIT})

###############################################################
### Set proper compile options if we already know about it  ###
###############################################################
if (FUTURES_PREFER_BOOST_DEPENDENCIES)
    target_compile_definitions(futures PUBLIC FUTURES_PREFER_BOOST_DEPENDENCIES)
endif()
if (FUTURES_PREFER_STANDALONE_DEPENDENCIES)
    target_compile_definitions(futures PUBLIC FUTURES_PREFER_STANDALONE_DEPENDENCIES)
endif()

###########################################################
### Target with separate compilation (not header-only)  ###
###########################################################
# This is a version of futures where ASIO is not header-only
add_library(futures-compiled
        futures-compiled/src.cpp
        )
target_link_libraries(futures-compiled PUBLIC futures)
target_compile_definitions(futures-compiled PUBLIC ASIO_SEPARATE_COMPILATION)
add_library(futures::futures-compiled ALIAS futures-compiled)

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
