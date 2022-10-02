//
// Copyright (c) 2022 alandefreitas (alandefreitas@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//

#ifndef FUTURES_DETAIL_SHARED_STATE_HPP
#define FUTURES_DETAIL_SHARED_STATE_HPP

#include <futures/detail/operation_state.hpp>
#include <memory>

namespace futures::detail {
    /** @addtogroup futures Futures
     *  @{
     */

    /// A shared operation state
    /**
     * Futures typically require their operation states to be shared
     *
     * @tparam R State data type
     * @tparam Options State options
     */
    template <class R, class Options>
    using shared_state = std::shared_ptr<operation_state<R, Options>>;

    template <class R, class Options>
    using deferred_shared_state = std::shared_ptr<
        deferred_operation_state<R, Options>>;

    /** @} */ // @addtogroup futures Futures
} // namespace futures::detail


#endif // FUTURES_DETAIL_SHARED_STATE_HPP
