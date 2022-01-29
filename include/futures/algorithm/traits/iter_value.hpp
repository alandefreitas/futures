//
// Copyright (c) 2022 alandefreitas (alandefreitas@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//

#ifndef FUTURES_ALGORITHM_TRAITS_ITER_VALUE_H
#define FUTURES_ALGORITHM_TRAITS_ITER_VALUE_H

#include <futures/algorithm/traits/has_element_type.hpp>
#include <futures/algorithm/traits/has_iterator_traits_value_type.hpp>
#include <futures/algorithm/traits/has_value_type.hpp>
#include <futures/algorithm/traits/remove_cvref.hpp>
#include <iterator>
#include <type_traits>

namespace futures {
    /** \addtogroup algorithms Algorithms
     *  @{
     */

    /** \addtogroup traits Traits
     *  @{
     */


    /** \brief A C++17 type trait equivalent to the C++20 iter_value
     * concept
     */
#ifdef FUTURES_DOXYGEN
    template <class T>
    using iter_value = __see_below__;
#else
    template <class T, class = void>
    struct iter_value
    {};

    template <class T>
    struct iter_value<
        T,
        std::enable_if_t<has_iterator_traits_value_type_v<remove_cvref_t<T>>>>
    {
        using type = typename std::iterator_traits<
            remove_cvref_t<T>>::value_type;
    };

    template <class T>
    struct iter_value<
        T,
        std::enable_if_t<
            !has_iterator_traits_value_type_v<
                remove_cvref_t<T>> && std::is_pointer_v<T>>>
    {
        using type = decltype(*std::declval<std::remove_cv_t<T>>());
    };

    template <class T>
    struct iter_value<
        T,
        std::enable_if_t<
            !has_iterator_traits_value_type_v<remove_cvref_t<
                T>> && !std::is_pointer_v<T> && std::is_array_v<T>>>
    {
        using type = std::remove_cv_t<std::remove_extent_t<T>>;
    };

    template <class T>
    struct iter_value<
        T,
        std::enable_if_t<
            !has_iterator_traits_value_type_v<remove_cvref_t<
                T>> && !std::is_pointer_v<T> && !std::is_array_v<T> && std::is_const_v<T>>>
    {
        using type = typename iter_value<std::remove_const_t<T>>::type;
    };

    template <class T>
    struct iter_value<
        T,
        std::enable_if_t<
            !has_iterator_traits_value_type_v<remove_cvref_t<
                T>> && !std::is_pointer_v<T> && !std::is_array_v<T> && !std::is_const_v<T> && has_value_type_v<T>>>
    {
        using type = typename T::value_type;
    };

    template <class T>
    struct iter_value<
        T,
        std::enable_if_t<
            !has_iterator_traits_value_type_v<remove_cvref_t<
                T>> && !std::is_pointer_v<T> && !std::is_array_v<T> && !std::is_const_v<T> && !has_value_type_v<T> && has_element_type_v<T>>>
    {
        using type = typename T::element_type;
    };

#endif
    template <class T>
    using iter_value_t = typename iter_value<T>::type;

    /** @}*/
    /** @}*/

} // namespace futures

#endif // FUTURES_ALGORITHM_TRAITS_ITER_VALUE_H
