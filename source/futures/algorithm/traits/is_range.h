//
// Copyright (c) 2022 alandefreitas (alandefreitas@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//

#ifndef FUTURES_ALGORITHM_TRAITS_IS_RANGES_H
#define FUTURES_ALGORITHM_TRAITS_IS_RANGES_H

#include <type_traits>

namespace futures {
    /** A C++17 type trait equivalent to the C++20 range concept
     */
#ifdef FUTURES_DOXYGEN
    template <class T>
    using is_range = __see_below__;
#else
    template <class T, class = void>
    struct is_range : std::false_type
    {};

    template <class T>
    struct is_range<
        T,
        std::void_t<
            decltype(*begin(std::declval<T>())),
            decltype(*end(std::declval<T>()))>> : std::true_type
    {};
#endif
    template <class T>
    bool constexpr is_range_v = is_range<T>::value;

} // namespace futures

#endif // FUTURES_ALGORITHM_TRAITS_IS_RANGES_H
