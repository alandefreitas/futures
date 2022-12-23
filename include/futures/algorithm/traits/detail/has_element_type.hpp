//
// Copyright (c) 2022 alandefreitas (alandefreitas@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//

#ifndef FUTURES_ALGORITHM_TRAITS_DETAIL_HAS_ELEMENT_TYPE_HPP
#define FUTURES_ALGORITHM_TRAITS_DETAIL_HAS_ELEMENT_TYPE_HPP

#include <futures/algorithm/traits/detail/nested_element_type.hpp>
#include <futures/detail/deps/boost/mp11/utility.hpp>
#include <type_traits>

namespace futures {
    namespace detail {
        template <class T>
        using has_element_type = detail::
            mp_valid<detail::nested_element_type_t, T>;
    } // namespace detail
} // namespace futures

#endif // FUTURES_ALGORITHM_TRAITS_DETAIL_HAS_ELEMENT_TYPE_HPP
