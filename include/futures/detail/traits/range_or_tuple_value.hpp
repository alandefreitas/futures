//
// Copyright (c) 2022 alandefreitas (alandefreitas@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//

#ifndef FUTURES_DETAIL_TRAITS_RANGE_OR_TUPLE_VALUE_HPP
#define FUTURES_DETAIL_TRAITS_RANGE_OR_TUPLE_VALUE_HPP

#include <futures/detail/traits/is_tuple.hpp>
#include <futures/algorithm/traits/is_range.hpp>
#include <futures/algorithm/traits/range_value.hpp>

namespace futures::detail {
    /// Get the element type of a when any result object
    /// This is a very specific helper trait we need
    template <typename T, class Enable = void>
    struct range_or_tuple_value
    {};

    template <typename Sequence>
    struct range_or_tuple_value<
        Sequence,
        std::enable_if_t<is_range_v<Sequence>>>
    {
        using type = range_value_t<Sequence>;
    };

    template <typename Sequence>
    struct range_or_tuple_value<
        Sequence,
        std::enable_if_t<detail::is_tuple_v<Sequence>>>
    {
        using type = std::tuple_element_t<0, Sequence>;
    };

    template <class T>
    using range_or_tuple_value_t = typename range_or_tuple_value<
        T>::type;
}

#endif // FUTURES_DETAIL_TRAITS_RANGE_OR_TUPLE_VALUE_HPP
