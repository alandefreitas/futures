//
// Copyright (c) 2021 alandefreitas (alandefreitas@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//

#ifndef FUTURES_ADAPTOR_DETAIL_LAMBDA_TO_FUTURE_HPP
#define FUTURES_ADAPTOR_DETAIL_LAMBDA_TO_FUTURE_HPP

namespace futures {
    namespace detail {
        // Trait to convert input type to its proper future type
        /*
         * - Futures become their decayed versions
         * - Lambdas become futures
         *
         * The primary template handles non-future types
         */
        template <class T, class Enable = void>
        struct lambda_to_future {
            using type = void;
        };

        // Trait to convert input type to its proper future type
        /*
         * (specialization for future types)
         *
         * - Futures become their decayed versions
         * - Lambdas become futures
         *
         * The primary template handles non-future types
         */
        template <typename Future>
        struct lambda_to_future<
            Future,
            std::enable_if_t<is_future_like_v<std::decay_t<Future>>>> {
            using type = std::decay_t<Future>;
        };

        // Trait to convert input type to its proper future type
        /* (specialization for functions)
         *
         *  - Futures become their decayed versions
         *  - Lambdas become futures
         *
         *  The primary template handles non-future types
         */
        template <typename Lambda>
        struct lambda_to_future<
            Lambda,
            std::enable_if_t<is_invocable_v<std::decay_t<Lambda>>>> {
            using type = futures::cfuture<invoke_result_t<std::decay_t<Lambda>>>;
        };

        template <class T>
        using lambda_to_future_t = typename lambda_to_future<T>::type;
    } // namespace detail
} // namespace futures

#endif // FUTURES_ADAPTOR_DETAIL_LAMBDA_TO_FUTURE_HPP
