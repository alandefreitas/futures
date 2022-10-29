//
// Copyright (c) 2021 alandefreitas (alandefreitas@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//

#ifndef FUTURES_DETAIL_SHARE_IF_NOT_SHARED_HPP
#define FUTURES_DETAIL_SHARE_IF_NOT_SHARED_HPP

#include <futures/traits/is_future.hpp>
#include <futures/traits/is_shared_future.hpp>

namespace futures::detail {
    /** @addtogroup futures Futures
     *  @{
     */
    /** @addtogroup future-traits Future Traits
     *  @{
     */

    /// Check if a type implements the share function
    /// This is what we use to identify the return type of a future type
    /// candidate However, this doesn't mean the type is a future in the
    /// terms of the is_future concept
    template <class T, typename = void>
    struct has_share : std::false_type {};

    template <class T>
    struct has_share<T, std::void_t<decltype(std::declval<T>().share())>>
        : std::true_type {};

    template <class Future>
    constexpr FUTURES_DETAIL(decltype(auto))
    share_if_not_shared(Future &&f) {
        if constexpr (is_shared_future_v<std::decay_t<Future>>) {
            return std::forward<Future>(f);
        } else if constexpr (detail::has_share<std::decay_t<Future>>::value) {
            return std::forward<Future>(f).share();
        } else {
            return std::move(std::forward<Future>(f));
        }
    }

    /** @} */ // @addtogroup future-traits Future Traits
    /** @} */ // @addtogroup futures Futures
} // namespace futures::detail
#endif // FUTURES_DETAIL_SHARE_IF_NOT_SHARED_HPP
