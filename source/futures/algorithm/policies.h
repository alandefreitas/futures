//
// Copyright (c) 2021 alandefreitas (alandefreitas@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//

#ifndef FUTURES_ALGORITHM_POLICIES_H
#define FUTURES_ALGORITHM_POLICIES_H

/// \file Identify traits for algorithms, like we do for other types
///
/// The traits help us generate auxiliary algorithm overloads
/// This is somewhat similar to the pattern of traits and algorithms for ranges
/// and views It allows us to get algorithm overloads for free, including
/// default inference of the best execution policies
///
/// \see https://en.cppreference.com/w/cpp/ranges/transform_view
/// \see https://en.cppreference.com/w/cpp/ranges/view
///

#include <execution>

#ifdef __has_include
#    if __has_include(<version>)
#        include <version>
#    endif
#endif

#include <futures/algorithm/partitioner/partitioner.h>
#include <futures/executor/default_executor.h>
#include <futures/executor/inline_executor.h>
#include <futures/algorithm/detail/traits/range/range/concepts.h>

namespace futures {
    /** \addtogroup algorithms Algorithms
     *  @{
     */

    /** \addtogroup execution-policies Execution Policies
     *  @{
     */

    /// Class representing a type for a sequenced_policy tag
    class sequenced_policy
    {};

    /// Class representing a type for a parallel_policy tag
    class parallel_policy
    {};

    /// Class representing a type for a parallel_unsequenced_policy tag
    class parallel_unsequenced_policy
    {};

    /// Class representing a type for an unsequenced_policy tag
    class unsequenced_policy
    {};

    /// @name Instances of the execution policy types

    /// \brief Tag used in algorithms for a sequenced_policy
    inline constexpr sequenced_policy seq{};

    /// \brief Tag used in algorithms for a parallel_policy
    inline constexpr parallel_policy par{};

    /// \brief Tag used in algorithms for a parallel_unsequenced_policy
    inline constexpr parallel_unsequenced_policy par_unseq{};

    /// \brief Tag used in algorithms for an unsequenced_policy
    inline constexpr unsequenced_policy unseq{};

    /// \brief Checks whether T is a standard or implementation-defined
    /// execution policy type.
    template <class T>
    struct is_execution_policy
        : std::disjunction<
              std::is_same<T, sequenced_policy>,
              std::is_same<T, parallel_policy>,
              std::is_same<T, parallel_unsequenced_policy>,
              std::is_same<T, unsequenced_policy>>
    {};

    /// \brief Checks whether T is a standard or implementation-defined
    /// execution policy type.
    template <class T>
    inline constexpr bool is_execution_policy_v = is_execution_policy<T>::value;

    /// \brief Make an executor appropriate to a given policy and a pair of
    /// iterators This depends, of course, of the default executors we have
    /// available and
    template <
        class E,
        class I,
        class S
#ifndef FUTURES_DOXYGEN
        ,
        std::enable_if_t<
            !is_executor_v<
                E> && is_execution_policy_v<E> && is_input_iterator_v<I> && futures::detail::sentinel_for<S, I>,
            int> = 0
#endif
        >
    constexpr decltype(auto)
    make_policy_executor() {
        if constexpr (!std::is_same_v<E, sequenced_policy>) {
            return make_default_executor();
        } else {
            return make_inline_executor();
        }
    }

    /** @}*/
    /** @}*/
} // namespace futures

#endif // FUTURES_ALGORITHM_POLICIES_H
