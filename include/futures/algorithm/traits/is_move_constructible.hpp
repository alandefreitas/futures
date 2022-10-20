//
// Copyright (c) 2022 alandefreitas (alandefreitas@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//

#ifndef FUTURES_ALGORITHM_TRAITS_IS_MOVE_CONSTRUCTIBLE_HPP
#define FUTURES_ALGORITHM_TRAITS_IS_MOVE_CONSTRUCTIBLE_HPP

#include <futures/algorithm/traits/is_constructible_from.hpp>
#include <futures/algorithm/traits/is_convertible_to.hpp>
#include <type_traits>

namespace futures {
    /** @addtogroup algorithms Algorithms
     *  @{
     */

    /** @addtogroup traits Traits
     *  @{
     */


    /** \brief A C++17 type trait equivalent to the C++20 move_constructible
     * concept
     */
    template <class T>
    using is_move_constructible = std::
        conjunction<is_constructible_from<T, T>, is_convertible_to<T, T>>;

    /// @copydoc is_move_constructible
    template <class T>
    constexpr bool is_move_constructible_v = is_move_constructible<T>::value;

    /** @} */
    /** @} */

} // namespace futures

#endif // FUTURES_ALGORITHM_TRAITS_IS_MOVE_CONSTRUCTIBLE_HPP
