//
// Copyright (c) 2022 alandefreitas (alandefreitas@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//

#ifndef FUTURES_ALGORITHM_TRAITS_DETAIL_HAS_DIFFERENCE_TYPE_HPP
#define FUTURES_ALGORITHM_TRAITS_DETAIL_HAS_DIFFERENCE_TYPE_HPP

#include <futures/algorithm/traits/detail/nested_difference_type.hpp>
#include <futures/detail/deps/boost/mp11/utility.hpp>
#include <type_traits>

namespace futures {
    namespace detail {
        template <class T>
        using has_difference_type = detail::
            mp_valid<detail::nested_difference_type_t, T>;

        template <class T>
        constexpr bool has_difference_type_v = has_difference_type<T>::value;
    } // namespace detail
} // namespace futures

#endif // FUTURES_ALGORITHM_TRAITS_DETAIL_HAS_DIFFERENCE_TYPE_HPP
