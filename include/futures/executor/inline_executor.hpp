//
// Copyright (c) 2021 alandefreitas (alandefreitas@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//

#ifndef FUTURES_EXECUTOR_INLINE_EXECUTOR_HPP
#define FUTURES_EXECUTOR_INLINE_EXECUTOR_HPP

/**
 *  @file executor/inline_executor.hpp
 *  @brief Inline executor
 *
 *  This file defines the inline executor, which executes tasks synchronously.
 */

#include <futures/config.hpp>
#include <futures/executor/is_executor.hpp>

namespace futures {
    /** @addtogroup executors Executors
     *  @{
     */

    /// An executor that runs anything inline
    /**
     * Although simple, it needs to meet the executor requirements:
     * - Executor concept
     * - Ability to query the execution context
     *     - Result being derived from execution_context
     * - The execute function
     * @see https://think-async.com/Asio/asio-1.18.2/doc/asio/std_executors.html
     */
    class inline_executor {
    public:
        constexpr inline_executor() = default;

        template <class F>
        void
        execute(F &&f) const {
            f();
        }
    };

    /// Make an inline executor object
    constexpr inline_executor
    make_inline_executor() {
        return {};
    }

    /** @} */ // @addtogroup executors Executors
} // namespace futures

#endif // FUTURES_EXECUTOR_INLINE_EXECUTOR_HPP
