//
// Created by Alan Freitas on 8/18/21.
//

#ifndef CPP_MANIFEST_UNWRAP_FUTURE_H
#define CPP_MANIFEST_UNWRAP_FUTURE_H

#include "is_future.h"
#include "has_get.h"

namespace futures {
    /// \section Get the type a future returns

    /// \struct Primary template handles non-future types
    template <typename T, class Enable = void> struct unwrap_future { using type = void; };

    /// \struct Template for types that implement ::get()
    template <typename Future>
    struct unwrap_future<Future, std::enable_if_t<detail::has_get<std::decay_t<Future>>::value>> {
        using type = std::invoke_result_t<decltype(&std::decay_t<Future>::get), Future>;
    };

    template <class T> using unwrap_future_t = typename unwrap_future<T>::type;

} // namespace futures

#endif // CPP_MANIFEST_UNWRAP_FUTURE_H
