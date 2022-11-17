//
// Copyright (c) 2022 alandefreitas (alandefreitas@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//

#ifndef FUTURES_FUTURE_OPTIONS_HPP
#define FUTURES_FUTURE_OPTIONS_HPP

#include <futures/future_options_args.hpp>
#include <futures/executor/default_executor.hpp>
#include <futures/detail/future_options_set.hpp>
#include <type_traits>

/**
 *  @file future_options.hpp
 *  @brief Future options
 *
 *  This file defines objects we can use to determine the compile-time options
 *  for a future type.
 */

namespace futures {
    /** @addtogroup futures Futures
     *  @{
     */
    /** @addtogroup future-options Future options
     *  @{
     *
     *  @brief Traits to define @ref basic_future types
     */

    template <class... Args>
    using future_options = detail::future_options_flat_t<Args...>;

    /** @} */
    /** @} */
} // namespace futures

#endif // FUTURES_FUTURE_OPTIONS_HPP
