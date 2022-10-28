//
// Copyright (c) 2021 alandefreitas (alandefreitas@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//

#ifndef FUTURES_EXECUTOR_IMPL_INLINE_EXECUTOR_IPP
#define FUTURES_EXECUTOR_IMPL_INLINE_EXECUTOR_IPP

#include <futures/executor/inline_executor.hpp>

namespace futures {
    asio::execution_context&
    inline_execution_context() {
        static asio::execution_context context;
        return context;
    }
} // namespace futures

#endif // FUTURES_EXECUTOR_IMPL_INLINE_EXECUTOR_IPP
