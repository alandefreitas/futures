//
// Copyright (c) 2021 alandefreitas (alandefreitas@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//

#ifndef FUTURES_FUTURES_DETAIL_SHARE_IF_NOT_SHARED_HPP
#define FUTURES_FUTURES_DETAIL_SHARE_IF_NOT_SHARED_HPP

#include <futures/futures/traits/is_future.hpp>
#include <futures/detail/traits/has_share.hpp>

namespace futures::detail {
    /** @addtogroup futures Futures
     *  @{
     */
    /** @addtogroup future-traits Future Traits
     *  @{
     */

    template <class Future>
    constexpr decltype(auto)
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
#endif // FUTURES_FUTURES_DETAIL_SHARE_IF_NOT_SHARED_HPP
