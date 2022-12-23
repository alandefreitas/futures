//
// Copyright (c) 2021 alandefreitas (alandefreitas@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//

#ifndef FUTURES_ADAPTOR_DETAIL_WHEN_ANY_HPP
#define FUTURES_ADAPTOR_DETAIL_WHEN_ANY_HPP

#include <type_traits>

#ifndef FUTURES_DOXYGEN
namespace futures {
    template <typename Sequence>
    struct when_any_result;
} // namespace futures
#endif

namespace futures {
    namespace detail {
        // Check if type is a when_any_result
        template <typename>
        struct is_when_any_result : std::false_type {};
        template <typename Sequence>
        struct is_when_any_result<when_any_result<Sequence>>
            : std::true_type {};
        template <typename Sequence>
        struct is_when_any_result<when_any_result<Sequence> const>
            : std::true_type {};
        template <typename Sequence>
        struct is_when_any_result<when_any_result<Sequence> &>
            : std::true_type {};
        template <typename Sequence>
        struct is_when_any_result<when_any_result<Sequence> &&>
            : std::true_type {};
        template <typename Sequence>
        struct is_when_any_result<when_any_result<Sequence> const &>
            : std::true_type {};
        template <class T>
        constexpr bool is_when_any_result_v = is_when_any_result<T>::value;
    } // namespace detail
} // namespace futures


#endif // FUTURES_ADAPTOR_DETAIL_WHEN_ANY_HPP
