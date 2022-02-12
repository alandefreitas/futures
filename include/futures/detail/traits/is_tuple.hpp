//
// Copyright (c) 2021 alandefreitas (alandefreitas@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//

#ifndef FUTURES_DETAIL_TRAITS_IS_TUPLE_HPP
#define FUTURES_DETAIL_TRAITS_IS_TUPLE_HPP

#include <tuple>
#include <type_traits>

namespace futures::detail {
    /** @addtogroup futures Futures
     *  @{
     */
    /** @addtogroup future-traits Future Traits
     *  @{
     */

    /// Check if type is a tuple
    template <typename>
    struct is_tuple : std::false_type
    {};

    template <typename... Args>
    struct is_tuple<std::tuple<Args...>> : std::true_type
    {};
    template <typename... Args>
    struct is_tuple<const std::tuple<Args...>> : std::true_type
    {};
    template <typename... Args>
    struct is_tuple<std::tuple<Args...> &> : std::true_type
    {};
    template <typename... Args>
    struct is_tuple<std::tuple<Args...> &&> : std::true_type
    {};
    template <typename... Args>
    struct is_tuple<const std::tuple<Args...> &> : std::true_type
    {};

    template <class T>
    constexpr bool is_tuple_v = is_tuple<T>::value;
    /** @} */ // @addtogroup future-traits Future Traits
    /** @} */ // @addtogroup futures Futures
} // namespace futures

#endif // FUTURES_DETAIL_TRAITS_IS_TUPLE_HPP
