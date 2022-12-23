//
// Copyright (c) 2021 alandefreitas (alandefreitas@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//

#ifndef FUTURES_EXECUTOR_IS_EXECUTOR_HPP
#define FUTURES_EXECUTOR_IS_EXECUTOR_HPP

/**
 *  @file executor/is_executor.hpp
 *  @brief Executor trait
 *
 *  This file defines the trait to identify whether a type represents an
 *  executor.
 */

#include <futures/config.hpp>
#include <futures/detail/traits/std_type_traits.hpp>
#include <futures/executor/detail/is_executor.hpp>
#include <futures/detail/deps/asio/is_executor.hpp>
#include <futures/detail/deps/asio/execution/executor.hpp>

namespace futures {
    /** @addtogroup executors Executors
     *  @{
     */

    /// Determine if type is an executor
    /**
     *  We only consider asio executors to be executors for now.
     *
     *  Future and previous executor models can be considered here, as long as
     *  their interface is the same as asio or we implement their respective
     *  traits to make @ref async work properly.
     *
     *  This trait might be adjusted to allow other executor types.
     **/
    template <class T>
    using is_executor =
#ifndef FUTURES_DOXYGEN
        detail::disjunction<
            asio::is_executor<T>,
            asio::execution::is_executor<T>,
            detail::has_execute<T>>;
#else
        __see_below__;
#endif


    /// Determine if type is an executor
    template <class T>
    constexpr bool is_executor_v = is_executor<T>::value;

    /** @} */ // @addtogroup executors Executors
} // namespace futures

#endif // FUTURES_EXECUTOR_IS_EXECUTOR_HPP
