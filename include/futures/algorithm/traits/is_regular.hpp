//
// Copyright (c) 2022 alandefreitas (alandefreitas@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//

#ifndef FUTURES_ALGORITHM_TRAITS_IS_REGULAR_HPP
#define FUTURES_ALGORITHM_TRAITS_IS_REGULAR_HPP

#include <futures/algorithm/traits/is_equality_comparable.hpp>
#include <futures/algorithm/traits/is_semiregular.hpp>
#include <type_traits>

namespace futures {
    /** @addtogroup algorithms Algorithms
     *  @{
     */

    /** @addtogroup traits Traits
     *  @{
     */

    /** \brief A C++17 type trait equivalent to the C++20 regular
     * concept
     */
    template <class T>
    using is_regular = std::
        conjunction<is_semiregular<T>, is_equality_comparable<T>>;

    /// @copydoc is_regular
    template <class T>
    constexpr bool is_regular_v = is_regular<T>::value;

    /** @} */
    /** @} */
} // namespace futures

#endif // FUTURES_ALGORITHM_TRAITS_IS_REGULAR_HPP
