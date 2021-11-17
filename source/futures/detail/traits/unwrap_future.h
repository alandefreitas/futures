//
// Created by Alan Freitas on 8/18/21.
//

#ifndef FUTURES_UNWRAP_FUTURE_H
#define FUTURES_UNWRAP_FUTURE_H

#include "is_future.h"
#include "has_get.h"

namespace futures {

    /// \brief Determine type the future object holds
    ///
    /// Primary template handles non-future types
    ///
    /// \note Not to be confused with continuation unwrapping
    template <typename T, class Enable = void> struct unwrap_future { using type = void; };

    /// \brief Determine type a future object holds (specialization for types that implement `get()`)
    ///
    /// Template for types that implement ::get()
    ///
    /// \note Not to be confused with continuation unwrapping
    template <typename Future>
    struct unwrap_future<Future, std::enable_if_t<detail::has_get<std::decay_t<Future>>::value>> {
        using type = std::invoke_result_t<decltype(&std::decay_t<Future>::get), Future>;
    };

    /// \brief Determine type a future object holds
    ///
    /// \note Not to be confused with continuation unwrapping
    template <class T> using unwrap_future_t = typename unwrap_future<T>::type;

} // namespace futures

#endif // FUTURES_UNWRAP_FUTURE_H
