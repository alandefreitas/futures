//
// Copyright (c) 2022 alandefreitas (alandefreitas@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//

#ifndef FUTURES_ALGORITHM_TRAITS_ITER_DIFFERENCE_HPP
#define FUTURES_ALGORITHM_TRAITS_ITER_DIFFERENCE_HPP

#include <futures/algorithm/traits/has_difference_type.hpp>
#include <futures/algorithm/traits/has_iterator_traits_difference_type.hpp>
#include <futures/algorithm/traits/is_subtractable.hpp>
#include <futures/algorithm/traits/remove_cvref.hpp>
#include <iterator>
#include <type_traits>

namespace futures {
    /** @addtogroup algorithms Algorithms
     *  @{
     */

    /** @addtogroup traits Traits
     *  @{
     */


    /** \brief A C++17 type trait equivalent to the C++20 iter_difference
     * concept
     */
#ifdef FUTURES_DOXYGEN
    template <class T>
    using iter_difference = __see_below__;
#else
    template <class T, class = void>
    struct iter_difference {};

    template <class T>
    struct iter_difference<
        T,
        std::enable_if_t<
            // clang-format off
            has_iterator_traits_difference_type_v<remove_cvref_t<T>>
            // clang-format on
            >> {
        using type = typename std::iterator_traits<
            remove_cvref_t<T>>::difference_type;
    };

    template <class T>
    struct iter_difference<
        T,
        std::enable_if_t<
            // clang-format off
            !has_iterator_traits_difference_type_v<remove_cvref_t<T>> &&
            std::is_pointer_v<T>
            // clang-format on
            >> {
        using type = std::ptrdiff_t;
    };

    template <class T>
    struct iter_difference<
        T,
        std::enable_if_t<
            // clang-format off
            !has_iterator_traits_difference_type_v<remove_cvref_t<T>> &&
            !std::is_pointer_v<T> &&
            std::is_const_v<T>
            // clang-format on
            >> {
        using type = typename iter_difference<std::remove_const_t<T>>::type;
    };

    template <class T>
    struct iter_difference<
        T,
        std::enable_if_t<
            // clang-format off
            !has_iterator_traits_difference_type_v<remove_cvref_t<T>> &&
            !std::is_pointer_v<T> &&
            !std::is_const_v<T> &&
            has_iterator_traits_difference_type_v<remove_cvref_t<T>>
            // clang-format on
            >> {
        using type = typename T::difference_type;
    };

    template <class T>
    struct iter_difference<
        T,
        std::enable_if_t<
            // clang-format off
            !has_iterator_traits_difference_type_v<remove_cvref_t<T>> &&
            !std::is_pointer_v<T> &&
            !std::is_const_v<T> &&
            !has_iterator_traits_difference_type_v<remove_cvref_t<T>> &&
            is_subtractable_v<remove_cvref_t<T>>
            // clang-format on
            >> {
        using type = std::make_signed_t<
            decltype(std::declval<T>() - std::declval<T>())>;
    };
#endif
    template <class T>
    using iter_difference_t = typename iter_difference<T>::type;

    /** @}*/
    /** @}*/
} // namespace futures

#endif // FUTURES_ALGORITHM_TRAITS_ITER_DIFFERENCE_HPP
