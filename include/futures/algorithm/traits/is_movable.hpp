//
// Copyright (c) 2022 alandefreitas (alandefreitas@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//

#ifndef FUTURES_ALGORITHM_TRAITS_IS_MOVABLE_HPP
#define FUTURES_ALGORITHM_TRAITS_IS_MOVABLE_HPP

#include <futures/algorithm/traits/is_assignable_from.hpp>
#include <futures/algorithm/traits/is_move_constructible.hpp>
#include <futures/algorithm/traits/is_swappable.hpp>
#include <type_traits>

namespace futures {
    /** @addtogroup algorithms Algorithms
     *  @{
     */

    /** @addtogroup traits Traits
     *  @{
     */

    /** \brief A C++17 type trait equivalent to the C++20 movable
     * concept
     */
    template <class T>
    using is_movable = std::conjunction<
        std::is_object<T>,
        is_move_constructible<T>,
        is_assignable_from<T&, T>,
        is_swappable<T>>;

    /// @copydoc is_movable
    template <class T>
    constexpr bool is_movable_v = is_movable<T>::value;

    /** @} */
    /** @} */
} // namespace futures

#endif // FUTURES_ALGORITHM_TRAITS_IS_MOVABLE_HPP
