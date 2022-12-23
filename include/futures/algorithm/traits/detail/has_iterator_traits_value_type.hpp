//
// Copyright (c) 2022 alandefreitas (alandefreitas@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//

#ifndef FUTURES_ALGORITHM_TRAITS_DETAIL_HAS_ITERATOR_TRAITS_VALUE_TYPE_HPP
#define FUTURES_ALGORITHM_TRAITS_DETAIL_HAS_ITERATOR_TRAITS_VALUE_TYPE_HPP

#include <futures/config.hpp>
#include <futures/detail/deps/boost/mp11/utility.hpp>
#include <iterator>
#include <type_traits>

namespace futures {
    namespace detail {
        template <class T>
        using nested_iterator_traits_value_type = typename std::iterator_traits<
            T>::value_type;

        template <class T>
        using has_iterator_traits_value_type
            = mp_valid<nested_iterator_traits_value_type, T>;

        template <class T>
        constexpr bool has_iterator_traits_value_type_v
            = has_iterator_traits_value_type<T>::value;
    } // namespace detail
} // namespace futures

#endif // FUTURES_ALGORITHM_TRAITS_DETAIL_HAS_ITERATOR_TRAITS_VALUE_TYPE_HPP
