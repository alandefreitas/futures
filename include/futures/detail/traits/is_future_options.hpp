//
// Copyright (c) 2021 alandefreitas (alandefreitas@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//

#ifndef FUTURES_DETAIL_TRAITS_IS_FUTURE_OPTIONS_HPP
#define FUTURES_DETAIL_TRAITS_IS_FUTURE_OPTIONS_HPP

#include <futures/future_options.hpp>
#include <type_traits>

namespace futures {
    namespace detail {
        // Check if type is an instantiation of future options
        template <typename>
        struct is_future_options : std::false_type {};

        template <typename... Args>
        struct is_future_options<future_options_list<Args...>>
            : std::true_type {};
        template <typename... Args>
        struct is_future_options<future_options_list<Args...> const>
            : std::true_type {};
        template <typename... Args>
        struct is_future_options<future_options_list<Args...> &>
            : std::true_type {};
        template <typename... Args>
        struct is_future_options<future_options_list<Args...> &&>
            : std::true_type {};
        template <typename... Args>
        struct is_future_options<future_options_list<Args...> const &>
            : std::true_type {};

        template <class T>
        constexpr bool is_future_options_v = is_future_options<T>::value;
        /** @} */ // @addtogroup future-traits Future Traits
        /** @} */ // @addtogroup futures Futures
    }             // namespace detail
} // namespace futures

#endif // FUTURES_DETAIL_TRAITS_IS_FUTURE_OPTIONS_HPP
