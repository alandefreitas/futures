//
// Copyright (c) 2021 alandefreitas (alandefreitas@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//

#ifndef FUTURES_AWAIT_H
#define FUTURES_AWAIT_H

#include <futures/futures/traits/is_future.hpp>
#include <type_traits>

namespace futures {
    /** \addtogroup futures Futures
     *  @{
     */

    /** \addtogroup waiting Waiting
     *
     * \brief Basic function to wait for futures
     *
     * This module defines a variety of auxiliary functions to wait for futures.
     *
     *  @{
     */

    /// \brief Very simple syntax sugar for types that pass the @ref is_future
    /// concept
    ///
    /// This syntax is most useful for cases where we are immediately requesting
    /// the future result.
    ///
    /// The function also makes the syntax optionally a little closer to
    /// languages such as javascript.
    ///
    /// \tparam Future A future type
    ///
    /// \return The result of the future object
    template <
        typename Future
#ifndef FUTURES_DOXYGEN
        ,
        std::enable_if_t<is_future_v<std::decay_t<Future>>, int> = 0
#endif
        >
    decltype(auto)
    await(Future &&f) {
        return f.get();
    }

    /** @} */
    /** @} */
} // namespace futures

#endif // FUTURES_AWAIT_H
