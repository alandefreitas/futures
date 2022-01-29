//
// Copyright (c) 2021 alandefreitas (alandefreitas@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//

#ifndef FUTURES_MOVE_OR_COPY_H
#define FUTURES_MOVE_OR_COPY_H

#include <futures/futures/traits/is_future.hpp>

namespace futures::detail {
    /** \addtogroup futures Futures
     *  @{
     */
    /** \addtogroup future-traits Future Traits
     *  @{
     */

    /// \brief Move or share a future, depending on the type of future input
    ///
    /// Create another future with the state of the before future (usually for a
    /// continuation function). This state should be copied to the new callback
    /// function. Shared futures can be copied. Normal futures should be moved.
    /// \return The moved future or the shared future
    template <class Future>
    constexpr decltype(auto)
    move_or_copy(Future &&before) {
        if constexpr (is_shared_future_v<Future>) {
            return std::forward<Future>(before);
        } else {
            return std::move(std::forward<Future>(before));
        }
    }

    /** @} */ // \addtogroup future-traits Future Traits
    /** @} */ // \addtogroup futures Futures
} // namespace futures::detail
#endif // FUTURES_MOVE_OR_COPY_H
