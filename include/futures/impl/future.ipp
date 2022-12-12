//
// Copyright (c) 2022 alandefreitas (alandefreitas@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//

#ifndef FUTURES_IMPL_FUTURE_IPP
#define FUTURES_IMPL_FUTURE_IPP

#include <futures/future.hpp>

#ifndef FUTURES_HEADER_ONLY
namespace futures {
    // extern templates we already know we need to instantiate to enable
    // other library functionality. For instance, future<void> is internally
    // used by multiple library functions.
    // future<void> (used in adaptors)
    extern template class basic_future<
        void,
        future_options<executor_opt<default_executor_type>>>;
    // cfuture<void> (used in adaptors)
    extern template class basic_future<
        void,
        future_options<executor_opt<default_executor_type>, continuable_opt>>;
    // cfuture<bool> (used in algorithms)
    extern template class basic_future<
        bool,
        future_options<executor_opt<default_executor_type>, continuable_opt>>;
} // namespace futures
#endif

#endif // FUTURES_IMPL_FUTURE_IPP
