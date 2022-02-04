//
// Copyright (c) 2021 alandefreitas (alandefreitas@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//

#ifndef FUTURES_FUTURES_TRAITS_FUTURE_VALUE_HPP
#define FUTURES_FUTURES_TRAITS_FUTURE_VALUE_HPP

#include <futures/futures/traits/is_future.hpp>
#include <futures/detail/traits/has_get.hpp>

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
    struct future_value
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
    struct future_value<
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
    using future_value_t = typename future_value<T>::type;

    /** @} */ // \addtogroup future-traits Future Traits
    /** @} */ // \addtogroup futures Futures
} // namespace futures

#endif // FUTURES_FUTURES_TRAITS_FUTURE_VALUE_HPP
