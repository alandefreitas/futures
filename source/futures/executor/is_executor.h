//
// Copyright (c) alandefreitas 12/9/21.
// See accompanying file LICENSE
//

#ifndef FUTURES_IS_EXECUTOR_H
#define FUTURES_IS_EXECUTOR_H

#include <futures/config/asio_include.h>


namespace futures {
    /** \addtogroup executors Executors
     *  @{
     */

    /// \brief Determine if type is an executor
    ///
    /// We only consider asio executors to be executors for now
    /// Future and previous executor models can be considered here, as long as their interface is the same
    /// as asio or we implement their respective traits to make @ref async work properly.
    template <typename T> using is_executor = asio::is_executor<T>;

    /// \brief Determine if type is an executor
    template <typename T> constexpr bool is_executor_v = is_executor<T>::value;

    /** @} */ // \addtogroup executors Executors
} // namespace futures

#endif // FUTURES_IS_EXECUTOR_H
