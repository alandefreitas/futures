//
// Copyright (c) 2021 alandefreitas (alandefreitas@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//

#ifndef FUTURES_EXECUTOR_IS_EXECUTOR_HPP
#define FUTURES_EXECUTOR_IS_EXECUTOR_HPP

#include <futures/config.hpp>
#include <futures/detail/deps/asio/is_executor.hpp>

namespace futures {
    /** @addtogroup executors Executors
     *  @{
     */

    /// Determine if type is an executor
    /**
     *  We only consider asio executors to be executors for now
     *  Future and previous executor models can be considered here, as long as
     *  their interface is the same as asio or we implement their respective
     *  traits to make @ref async work properly.
     **/
    template <typename T>
    using is_executor = asio::is_executor<T>;

    /// Determine if type is an executor
    template <typename T>
    constexpr bool is_executor_v = is_executor<T>::value;

    /** @} */ // @addtogroup executors Executors
} // namespace futures

#endif // FUTURES_EXECUTOR_IS_EXECUTOR_HPP
