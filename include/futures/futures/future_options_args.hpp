//
// Copyright (c) 2022 alandefreitas (alandefreitas@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//

#ifndef FUTURES_FUTURES_FUTURE_OPTIONS_ARGS_HPP
#define FUTURES_FUTURES_FUTURE_OPTIONS_ARGS_HPP

namespace futures {
    /** @addtogroup futures Futures
     *  @{
     */
    /** @addtogroup future-options Future options
     *  @{
     */

    struct continuable_opt
    {};

    struct stoppable_opt
    {};

    struct always_detached_opt
    {};

    struct always_deferred_opt
    {};

    template <class Fn>
    struct executor_opt
    {
        using type = Fn;
    };

    struct shared_opt
    {};

    /** @} */
    /** @} */
} // namespace futures


#endif // FUTURES_FUTURES_FUTURE_OPTIONS_ARGS_HPP
