//
// Copyright (c) 2022 alandefreitas (alandefreitas@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//

#ifndef FUTURES_ADAPTOR_DETAIL_RANGE_OR_TUPLE_VALUE_HPP
#define FUTURES_ADAPTOR_DETAIL_RANGE_OR_TUPLE_VALUE_HPP

#include <futures/algorithm/traits/is_range.hpp>
#include <futures/algorithm/traits/range_value.hpp>
#include <futures/detail/deps/boost/core/empty_value.hpp>
#include <futures/detail/deps/boost/mp11/algorithm.hpp>
#include <futures/detail/deps/boost/mp11/tuple.hpp>
#include <futures/detail/deps/boost/mp11/function.hpp>

namespace futures::detail {
    // Get the element type of a when_any_result Sequence object
    // This is a very specific helper trait we need when all elements in a
    // tuple are the same type, which enables extra continuations
    template <class Sequence>
    using range_or_tuple_value = mp_cond<
        // clang-format off
            is_range<Sequence>,                         range_value<Sequence>,
            detail::mp_similar<std::tuple<>, Sequence>, mp_defer<mp_front, Sequence>,
            std::true_type,                             boost::empty_init_t
        // clang-format on
        >;

    template <class T>
    using range_or_tuple_value_t = typename range_or_tuple_value<T>::type;
} // namespace futures::detail

#endif // FUTURES_ADAPTOR_DETAIL_RANGE_OR_TUPLE_VALUE_HPP
