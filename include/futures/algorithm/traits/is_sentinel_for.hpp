//
// Copyright (c) 2022 alandefreitas (alandefreitas@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//

#ifndef FUTURES_ALGORITHM_TRAITS_IS_SENTINEL_FOR_HPP
#define FUTURES_ALGORITHM_TRAITS_IS_SENTINEL_FOR_HPP

/**
 *  @file algorithm/traits/is_sentinel_for.hpp
 *  @brief `is_sentinel_for` trait
 *
 *  This file defines the `is_sentinel_for` trait.
 */

#include <futures/algorithm/traits/is_input_or_output_iterator.hpp>
#include <futures/algorithm/traits/is_semiregular.hpp>
#include <type_traits>

namespace futures {
    /** @addtogroup algorithms Algorithms
     *  @{
     */

    /** @addtogroup traits Traits
     *  @{
     */

    /// @brief A type trait equivalent to the `std::sentinel_for` concept
    /**
     * @see https://en.cppreference.com/w/cpp/iterator/sentinel_for
     */
    template <class S, class I>
    using is_sentinel_for = std::
        conjunction<is_semiregular<S>, is_input_or_output_iterator<I>>;

    /// @copydoc is_sentinel_for
    template <class S, class I>
    constexpr bool is_sentinel_for_v = is_sentinel_for<S, I>::value;

    /** @} */
    /** @} */
} // namespace futures

#endif // FUTURES_ALGORITHM_TRAITS_IS_SENTINEL_FOR_HPP
