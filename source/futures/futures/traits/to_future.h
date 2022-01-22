//
// Copyright (c) 2021 alandefreitas (alandefreitas@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//

#ifndef FUTURES_TO_FUTURE_H
#define FUTURES_TO_FUTURE_H

namespace futures {
    /// \brief Trait to convert input type to its proper future type
    ///
    /// - Futures become their decayed versions
    /// - Lambdas become futures
    ///
    /// The primary template handles non-future types
    template <typename T, class Enable = void>
    struct to_future
    {
        using type = void;
    };

    /// \brief Trait to convert input type to its proper future type
    /// (specialization for future types)
    ///
    /// - Futures become their decayed versions
    /// - Lambdas become futures
    ///
    /// The primary template handles non-future types
    template <typename Future>
    struct to_future<Future, std::enable_if_t<is_future_v<std::decay_t<Future>>>>
    {
        using type = std::decay_t<Future>;
    };

    /// \brief Trait to convert input type to its proper future type
    /// (specialization for functions)
    ///
    /// - Futures become their decayed versions
    /// - Lambdas become futures
    ///
    /// The primary template handles non-future types
    template <typename Lambda>
    struct to_future<
        Lambda,
        std::enable_if_t<std::is_invocable_v<std::decay_t<Lambda>>>>
    {
        using type = futures::cfuture<
            std::invoke_result_t<std::decay_t<Lambda>>>;
    };

    template <class T>
    using to_future_t = typename to_future<T>::type;
} // namespace futures

#endif // FUTURES_TO_FUTURE_H
