//
// Copyright (c) 2021 alandefreitas (alandefreitas@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//

#ifndef FUTURES_UNWRAP_FUTURE_H
#define FUTURES_UNWRAP_FUTURE_H

#include <futures/futures/traits/is_future.hpp>
#include <futures/adaptor/detail/traits/has_get.hpp>

namespace futures {
    /** \addtogroup futures Futures
     *  @{
     */
    /** \addtogroup future-traits Future Traits
     *  @{
     */


    /// \brief Determine type the future object holds
    ///
    /// Primary template handles non-future types
    ///
    /// \note Not to be confused with continuation unwrapping
    template <typename T, class Enable = void>
    struct unwrap_future
    {
        using type = void;
    };

    /// \brief Determine type a future object holds (specialization for types
    /// that implement `get()`)
    ///
    /// Template for types that implement ::get()
    ///
    /// \note Not to be confused with continuation unwrapping
    template <typename Future>
    struct unwrap_future<
        Future,
        std::enable_if_t<detail::has_get<std::decay_t<Future>>::value>>
    {
        using type = std::decay_t<
            decltype(std::declval<std::decay_t<Future>>().get())>;
    };

    /// \brief Determine type a future object holds
    ///
    /// \note Not to be confused with continuation unwrapping
    template <class T>
    using unwrap_future_t = typename unwrap_future<T>::type;

    /** @} */ // \addtogroup future-traits Future Traits
    /** @} */ // \addtogroup futures Futures
} // namespace futures

#endif // FUTURES_UNWRAP_FUTURE_H