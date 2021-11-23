#######################################################
### Futures library                                 ###
#######################################################
add_library(futures
        # Type aliases
        futures/config/asio_include.h
        futures/config/small_vector_include.h

        # Detail / Traits
        futures/adaptor/detail/traits/is_callable.h
        futures/adaptor/detail/traits/is_executor_then_continuation.h
        futures/adaptor/detail/traits/is_executor_then_function.h
        futures/adaptor/detail/traits/is_reference_wrapper.h
        futures/adaptor/detail/traits/is_tuple.h

        # Detail / Range Traits / Bundled subset from range-v3
        futures/algorithm/detail/traits/range/compare.hpp
        futures/algorithm/detail/traits/range/concepts/concepts.hpp
        futures/algorithm/detail/traits/range/concepts/type_traits.hpp
        futures/algorithm/detail/traits/range/concepts/swap.hpp
        futures/algorithm/detail/traits/range/concepts/compare.hpp
        futures/algorithm/detail/traits/range/detail/adl_get.hpp
        futures/algorithm/detail/traits/range/detail/config.hpp
        futures/algorithm/detail/traits/range/detail/range_access.hpp
        futures/algorithm/detail/traits/range/detail/prologue.hpp
        futures/algorithm/detail/traits/range/detail/epilogue.hpp
        futures/algorithm/detail/traits/range/functional/concepts.hpp
        futures/algorithm/detail/traits/range/functional/invoke.hpp
        futures/algorithm/detail/traits/range/functional/comparisons.hpp
        futures/algorithm/detail/traits/range/functional/reference_wrapper.hpp
        futures/algorithm/detail/traits/range/functional/identity.hpp
        futures/algorithm/detail/traits/range/iterator/concepts.hpp
        futures/algorithm/detail/traits/range/iterator/access.hpp
        futures/algorithm/detail/traits/range/iterator/traits.hpp
        futures/algorithm/detail/traits/range/iterator/basic_iterator.hpp
        futures/algorithm/detail/traits/range/iterator/reverse_iterator.hpp
        futures/algorithm/detail/traits/range/meta/meta_fwd.hpp
        futures/algorithm/detail/traits/range/meta/meta.hpp
        futures/algorithm/detail/traits/range/range/concepts.hpp
        futures/algorithm/detail/traits/range/range/access.hpp
        futures/algorithm/detail/traits/range/range/traits.hpp
        futures/algorithm/detail/traits/range/range/primitives.hpp
        futures/algorithm/detail/traits/range/range_fwd.hpp
        futures/algorithm/detail/traits/range/std/detail/associated_types.hpp
        futures/algorithm/detail/traits/range/utility/get.hpp
        futures/algorithm/detail/traits/range/utility/move.hpp
        futures/algorithm/detail/traits/range/utility/common_type.hpp
        futures/algorithm/detail/traits/range/utility/swap.hpp
        futures/algorithm/detail/traits/range/utility/addressof.hpp
        futures/algorithm/detail/traits/range/utility/in_place.hpp
        futures/algorithm/detail/traits/range/utility/static_const.hpp
        futures/algorithm/detail/traits/range/utility/semiregular_box.hpp
        futures/algorithm/detail/traits/range/utility/box.hpp
        futures/algorithm/detail/traits/range/version.hpp

        # Traits
        futures/futures/traits/future_return.h
        futures/futures/traits/is_future.h
        futures/futures/traits/is_future_continuation.h
        futures/futures/traits/to_future.h
        futures/futures/traits/unwrap_future.h

        # Better default executors for async
        futures/executor/default_executor.h
        futures/executor/default_executor.cpp
        futures/executor/inline_executor.h
        futures/executor/inline_executor.cpp

        # Future types
        futures/futures/stop_token.h
        futures/futures/basic_future.h

        # Adaptors
        futures/adaptor/detail/tuple_algorithm.h
        futures/adaptor/when_all.h
        futures/adaptor/when_any.h
        futures/adaptor/when_any_result.h
        futures/adaptor/then.h
        futures/adaptor/ready_future.h

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
