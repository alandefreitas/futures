//
// Copyright (c) 2022 alandefreitas (alandefreitas@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//

#ifndef FUTURES_ALGORITHM_TRAITS_ITERATOR_HPP
#define FUTURES_ALGORITHM_TRAITS_ITERATOR_HPP

#include <futures/algorithm/traits/remove_cvref.hpp>
#include <futures/algorithm/traits/detail/has_element_type.hpp>
#include <futures/algorithm/traits/detail/has_iterator_traits_value_type.hpp>
#include <futures/algorithm/traits/detail/has_value_type.hpp>
#include <iterator>
#include <type_traits>

namespace futures {
    /** @addtogroup algorithms Algorithms
     *  @{
     */

    /** @addtogroup traits Traits
     *  @{
     */


    /** \brief A C++17 type trait equivalent to the C++20 iterator_t
     * concept
     */
#ifdef FUTURES_DOXYGEN
    template <class T>
    using iterator = __see_below__;
#else
    template <class T, class = void>
    struct iterator {};

    template <class T>
    struct iterator<T, std::void_t<decltype(begin(std::declval<T&>()))>> {
        using type = decltype(begin(std::declval<T&>()));
    };
#endif

    /// @copydoc iterator
    template <class T>
    using iterator_t = typename iterator<T>::type;

    /** @} */
    /** @} */
} // namespace futures

#endif // FUTURES_ALGORITHM_TRAITS_ITERATOR_HPP
