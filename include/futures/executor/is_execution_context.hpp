//
// Copyright (c) 2023 alandefreitas (alandefreitas@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//

#ifndef FUTURES_EXECUTOR_IS_EXECUTION_CONTEXT_HPP
#define FUTURES_EXECUTOR_IS_EXECUTION_CONTEXT_HPP

/**
 *  @file executor/is_execution_context.hpp
 *  @brief Execution context traits
 *
 *  This file defines the trait to identify whether a type represents an
 *  executor.
 */

#include <futures/config.hpp>
#include <futures/executor/is_executor.hpp>
#include <futures/detail/traits/std_type_traits.hpp>
#include <type_traits>

namespace futures {
    /** @addtogroup executors Executors
     *  @{
     */

#ifdef FUTURES_HAS_CONCEPTS
    // clang-format off
    /// @concept execution_context_for
    /// @brief Determines if a type is an execution context for the a task type
    template <class C, class F>
    concept execution_context_for =
        requires(C ctx, F f) {
            { ctx.get_executor() } -> executor_for<F>;
        };
    // clang-format on

    /// @concept executor
    /// @brief Determines if a type is an execution context for invocable types
    /**
     *  The invocable archetype task is a regular functor. This means this trait
     *  should work for any execution context that supports non-heterogeneous
     *  tasks.
     **/
    template <class C>
    concept execution_context
        = execution_context_for<C, FUTURES_INVOCABLE_ARCHETYPE>;
#endif

    /// Determine if type is an execution context for the specified type of task
#ifdef FUTURES_HAS_CONCEPTS
    template <class E, class F>
    using is_execution_context_for = std::bool_constant<
        execution_context_for<E, F>>;
#else
    template <class E, class F>
    using is_execution_context_for = detail::has_get_executor<E>;
#endif

    /// Determines if a type is an execution context for invocable types
#ifdef FUTURES_HAS_CONCEPTS
    template <class E>
    using is_execution_context = std::bool_constant<execution_context<E>>;
#else
    template <class E>
    using is_execution_context
        = is_execution_context_for<E, detail::invocable_archetype>;
#endif

    /// @copydoc is_execution_context_for
#ifdef FUTURES_HAS_CONCEPTS
    template <class E, class F>
    constexpr bool is_execution_context_for_v = execution_context_for<E, F>;
#else
    template <class E, class F>
    constexpr bool is_execution_context_for_v = is_execution_context_for<E, F>::
        value;
#endif

    /// @copydoc is_execution_context
#ifdef FUTURES_HAS_CONCEPTS
    template <class E>
    constexpr bool is_execution_context_v = execution_context<E>;
#else
    template <class E>
    constexpr bool is_execution_context_v = is_execution_context<E>::value;
#endif

    /** @} */ // @addtogroup executors Executors
} // namespace futures

#endif // FUTURES_EXECUTOR_IS_EXECUTION_CONTEXT_HPP
