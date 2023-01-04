//
// Copyright (c) 2021 alandefreitas (alandefreitas@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//

#ifndef FUTURES_ALGORITHM_POLICIES_HPP
#define FUTURES_ALGORITHM_POLICIES_HPP

/**
 *  @file algorithm/policies.hpp
 *  @brief Algorithm execution policies
 *
 *  This file defines the policies we can use to determine the appropriate
 *  executor for algorithms.
 *
 *  The traits help us generate auxiliary algorithm overloads
 *  This is somewhat similar to the pattern of traits and algorithms for ranges
 *  and views It allows us to get algorithm overloads for free, including
 *  default inference of the best execution policies
 *
 *  @see
 * [`std::ranges::transform_view`](https://en.cppreference.com/w/cpp/ranges/transform_view)
 *  @see [`std::ranges::view`](https://en.cppreference.com/w/cpp/ranges/view)
 */

#include <futures/algorithm/partitioner/partitioner_for.hpp>
#include <futures/algorithm/traits/is_sentinel_for.hpp>
#include <futures/executor/default_executor.hpp>
#include <futures/executor/inline_executor.hpp>
#include <futures/detail/traits/std_type_traits.hpp>
#include <futures/algorithm/detail/execution.hpp>

namespace futures {
    /** @addtogroup algorithms Algorithms
     *  @{
     */

    /** @addtogroup execution-policies Execution Policies
     *  @{
     */

    /// Class representing a type for a sequenced_policy tag
    /**
     * This tag type is an alias to `std::execution::sequenced_policy` whenever
     * it's available. Otherwise it's defined as an individual type.
     */
#ifdef FUTURES_HAS_STD_POLICIES
    using sequenced_policy = std::execution::sequenced_policy;
#else
    class sequenced_policy {};
#endif

    /// Class representing a type for a parallel_policy tag
    /**
     * This tag type is an alias to `std::execution::parallel_policy` whenever
     * it's available. Otherwise it's defined as an individual type.
     */
#ifdef FUTURES_HAS_STD_POLICIES
    using parallel_policy = std::execution::parallel_policy;
#else
    class parallel_policy {};
#endif

    /// Class representing a type for a parallel_unsequenced_policy tag
    /**
     * This tag type is an alias to
     * `std::execution::parallel_unsequenced_policy` whenever it's available.
     * Otherwise it's defined as an individual type.
     */
#ifdef FUTURES_HAS_STD_POLICIES
    using parallel_unsequenced_policy = std::execution::
        parallel_unsequenced_policy;
#else
    class parallel_unsequenced_policy {};
#endif

    /// Class representing a type for an unsequenced_policy tag
    /**
     * This tag type is an alias to `std::execution::unsequenced_policy`
     * whenever it's available. Otherwise it's defined as an individual type.
     */
#ifdef FUTURES_HAS_STD_POLICIES
    using unsequenced_policy = std::execution::unsequenced_policy;
#else
    class unsequenced_policy {};
#endif

    /// Tag used in algorithms for a sequenced_policy
    FUTURES_INLINE_VAR constexpr sequenced_policy seq{};

    /// Tag used in algorithms for a parallel_policy
    FUTURES_INLINE_VAR constexpr parallel_policy par{};

    /// Tag used in algorithms for a parallel_unsequenced_policy
    FUTURES_INLINE_VAR constexpr parallel_unsequenced_policy par_unseq{};

    /// Tag used in algorithms for an unsequenced_policy
    FUTURES_INLINE_VAR constexpr unsequenced_policy unseq{};

    /// Determines whether T is a standard or implementation-defined
    /// execution policy type.
    template <class T>
    struct is_execution_policy
        : detail::disjunction<
              std::is_same<T, sequenced_policy>,
              std::is_same<T, parallel_policy>,
              std::is_same<T, parallel_unsequenced_policy>,
              std::is_same<T, unsequenced_policy>> {};

    /// @copydoc is_execution_policy
    template <class T>
    FUTURES_INLINE_VAR constexpr bool is_execution_policy_v
        = is_execution_policy<T>::value;

#ifdef FUTURES_HAS_CONCEPTS
    /// @concept execution_policy
    /// @brief Determines if a type is an execution_policy
    template <class E>
    concept execution_policy = is_execution_policy_v<E>;
#endif
    /** @} */
    /** @} */
} // namespace futures

#endif // FUTURES_ALGORITHM_POLICIES_HPP
