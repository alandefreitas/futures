//
// Copyright (c) 2022 alandefreitas (alandefreitas@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//

#ifndef FUTURES_IMPL_IS_READY_HPP
#define FUTURES_IMPL_IS_READY_HPP

namespace futures {
    FUTURES_TEMPLATE_IMPL(class Future)
    (requires is_future_v<std::decay_t<Future>>) bool is_ready(Future &&f) {
        assert(
            f.valid()
            && "Undefined behaviour. Checking if an invalid future is ready.");
        if constexpr (detail::has_is_ready_v<Future>) {
            return f.is_ready();
        } else {
            return f.wait_for(std::chrono::seconds(0))
                   == std::future_status::ready;
        }
    }
} // namespace futures

#endif // FUTURES_IMPL_IS_READY_HPP
