//
// Copyright (c) alandefreitas 12/4/21.
// See accompanying file LICENSE
//

#ifndef FUTURES_IS_TUPLE_INVOCABLE_H
#define FUTURES_IS_TUPLE_INVOCABLE_H

#include <tuple>
#include <type_traits>

namespace futures::detail {
    /** \addtogroup futures Futures
     *  @{
     */

    /// \brief Check if a function can be invoked with the elements of a tuple as arguments, as in std::apply
    template <typename Function, typename Tuple>
    struct is_tuple_invocable : std::false_type {};

    template <typename Function, class... Args>
    struct is_tuple_invocable<Function, std::tuple<Args...>>
        : std::is_invocable<Function, Args...> {};

    template <typename Function, typename Tuple>
    constexpr bool is_tuple_invocable_v = is_tuple_invocable<Function, Tuple>::value;

    /** @} */
} // namespace futures::detail

#endif // FUTURES_IS_TUPLE_INVOCABLE_H
