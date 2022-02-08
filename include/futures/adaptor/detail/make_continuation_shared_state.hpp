//
// Copyright (c) 2022 alandefreitas (alandefreitas@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//

#ifndef FUTURES_ADAPTOR_DETAIL_MAKE_CONTINUATION_SHARED_STATE_HPP
#define FUTURES_ADAPTOR_DETAIL_MAKE_CONTINUATION_SHARED_STATE_HPP

#include <futures/futures/detail/shared_state.hpp>

namespace futures::detail {
    template <
        class value_type,
        class future_options,
        class Executor,
        class Function>
    std::shared_ptr<shared_state<value_type, future_options>>
    make_continuation_shared_state(const Executor &ex, Function &&f) {
        if constexpr (!future_options::is_always_deferred) {
            using shared_state_t = shared_state<value_type, future_options>;
            (void) f;
            return std::make_shared<shared_state_t>(ex);
        } else {
            using shared_state_t
                = deferred_shared_state<value_type, future_options, Function>;
            return std::make_shared<
                shared_state_t>(ex, std::forward<Function>(f));
        }
    }
} // namespace futures::detail

#endif // FUTURES_ADAPTOR_DETAIL_MAKE_CONTINUATION_SHARED_STATE_HPP
