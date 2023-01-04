//
// Copyright (c) 2023 alandefreitas (alandefreitas@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//

#ifndef FUTURES_EXECUTOR_IS_EXECUTOR_HPP
#define FUTURES_EXECUTOR_IS_EXECUTOR_HPP

/**
 *  @file executor/is_executor.hpp
 *  @brief Executor traits
 *
 *  This file defines the trait to identify whether a type represents an
 *  executor.
 */

#include <futures/config.hpp>
#include <futures/detail/traits/std_type_traits.hpp>
#include <futures/executor/detail/is_executor.hpp>
#include <type_traits>

namespace futures {
    /** @addtogroup executors Executors
     *  @{
     */

#ifdef FUTURES_HAS_CONCEPTS
    /// @concept executor_for
    /// @brief Determines if a type is an executor for the specified type of task
#    ifdef FUTURES_DOXYGEN
    template <class E, class F>
    concept executor_for = requires(E e, F f) { e.execute(f); };
#    else
    // the implementation also experimentally supports asio executors
    template <class E, class F>
    concept executor_for = requires(E e, F f) { e.execute(f); }
                           || requires(E e, F f) {
                                  e.context();
                                  e.on_work_started();
                                  e.on_work_finished();
                                  e.dispatch(f, std::allocator<void>{});
                                  e.post(f, std::allocator<void>{});
                                  e.defer(f, std::allocator<void>{});
                              };
#    endif

    /// @concept executor
    /// @brief Determines if a type is an executor for invocable types
    /**
     *  The invocable archetype task is a regular functor. This means this trait
     *  should work for any executor that supports non-heterogeneous tasks.
     **/
    template <class E>
    concept executor = executor_for<E, FUTURES_INVOCABLE_ARCHETYPE>;
#endif

    /// Determine if type is an executor for the specified type of task
#ifdef FUTURES_HAS_CONCEPTS
    template <class E, class F>
    using is_executor_for = std::bool_constant<executor_for<E, F>>;
#else
    template <class E, class F>
    using is_executor_for = detail::disjunction<
        detail::is_executor_for_impl<E, F>,
        detail::is_asio_executor_for<E, F>>;
#endif

    /// Determines if a type is an executor for invocable types
#ifdef FUTURES_HAS_CONCEPTS
    template <class E>
    using is_executor = std::bool_constant<executor<E>>;
#else
    template <class E>
    using is_executor = is_executor_for<E, detail::invocable_archetype>;
#endif

    /// @copydoc is_executor_for
#ifdef FUTURES_HAS_CONCEPTS
    template <class E, class F>
    constexpr bool is_executor_for_v = executor_for<E, F>;
#else
    template <class E, class F>
    constexpr bool is_executor_for_v = is_executor_for<E, F>::value;
#endif

    /// @copydoc is_executor
#ifdef FUTURES_HAS_CONCEPTS
    template <class E>
    constexpr bool is_executor_v = executor<E>;
#else
    template <class E>
    constexpr bool is_executor_v = is_executor<E>::value;
#endif

    /** @} */ // @addtogroup executors Executors
} // namespace futures

#endif // FUTURES_EXECUTOR_IS_EXECUTOR_HPP
