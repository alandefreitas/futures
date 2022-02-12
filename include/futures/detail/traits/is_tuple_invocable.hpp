//
// Copyright (c) 2021 alandefreitas (alandefreitas@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//

#ifndef FUTURES_DETAIL_TRAITS_IS_TUPLE_INVOCABLE_HPP
#define FUTURES_DETAIL_TRAITS_IS_TUPLE_INVOCABLE_HPP

#include <tuple>
#include <type_traits>

namespace futures::detail {
    /** @addtogroup futures Futures
     *  @{
     */

    /// Check if a function can be invoked with the elements of a tuple
    /// as arguments, as in std::apply
    template <typename Function, typename Tuple>
    struct is_tuple_invocable : std::false_type
    {};

    template <typename Function, class... Args>
    struct is_tuple_invocable<Function, std::tuple<Args...>>
        : std::is_invocable<Function, Args...>
    {};

    template <typename Function, typename Tuple>
    constexpr bool is_tuple_invocable_v = is_tuple_invocable<Function, Tuple>::
        value;

    /** @} */
} // namespace futures::detail

#endif // FUTURES_DETAIL_TRAITS_IS_TUPLE_INVOCABLE_HPP
