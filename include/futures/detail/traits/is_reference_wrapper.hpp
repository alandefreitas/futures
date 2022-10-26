//
// Copyright (c) 2022 alandefreitas (alandefreitas@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//

#ifndef FUTURES_DETAIL_TRAITS_IS_REFERENCE_WRAPPER_HPP
#define FUTURES_DETAIL_TRAITS_IS_REFERENCE_WRAPPER_HPP

#include <type_traits>

namespace futures::detail {
    // Check if type is a reference_wrapper
    template <typename>
    struct is_reference_wrapper : std::false_type {};

    template <class T>
    struct is_reference_wrapper<std::reference_wrapper<T>> : std::true_type {};

    template <class T>
    constexpr bool is_reference_wrapper_v = is_reference_wrapper<T>::value;
} // namespace futures::detail

#endif // FUTURES_DETAIL_TRAITS_IS_REFERENCE_WRAPPER_HPP
