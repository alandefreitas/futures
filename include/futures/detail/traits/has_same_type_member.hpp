//
// Copyright (c) 2022 alandefreitas (alandefreitas@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//

#ifndef FUTURES_DETAIL_TRAITS_HAS_SAME_TYPE_MEMBER_HPP
#define FUTURES_DETAIL_TRAITS_HAS_SAME_TYPE_MEMBER_HPP

#include <type_traits>

namespace futures::detail {
    template <class T, class U, class = void>
    struct has_same_type_member : std::false_type
    {};

    template <class T, class U>
    struct has_same_type_member<
        T,
        U,
        std::void_t<typename T::type, typename U::type>>
        : std::is_same<typename T::type, typename U::type>
    {};

    template <class T, class U>
    constexpr bool has_same_type_member_v = has_same_type_member<T, U>::value;

} // namespace futures::detail

#endif // FUTURES_DETAIL_TRAITS_HAS_SAME_TYPE_MEMBER_HPP
