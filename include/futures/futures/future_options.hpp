//
// Copyright (c) 2022 alandefreitas (alandefreitas@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//

#ifndef FUTURES_FUTURES_FUTURE_OPTIONS_HPP
#define FUTURES_FUTURES_FUTURE_OPTIONS_HPP

#include <futures/executor/default_executor.hpp>
#include <futures/futures/future_options_args.hpp>
#include <futures/futures/detail/future_options_set.hpp>
#include <futures/futures/detail/traits/get_type_template_in_args.hpp>
#include <futures/futures/detail/traits/index_in_args.hpp>
#include <futures/futures/detail/traits/is_in_args.hpp>
#include <futures/futures/detail/traits/is_type_template_in_args.hpp>
#include <type_traits>

namespace futures {
    /** @addtogroup futures Futures
     *  @{
     */
    /** @addtogroup future-options Future options
     *  @{
     */

    template <class... Args>
    using future_options = detail::future_options_flat_t<Args...>;

    /** @} */
    /** @} */
} // namespace futures

#endif // FUTURES_FUTURES_FUTURE_OPTIONS_HPP
