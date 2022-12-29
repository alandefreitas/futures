//
// Copyright (c) 2022 alandefreitas (alandefreitas@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//

#ifndef FUTURES_EXECUTOR_DETAIL_EXECUTE_HPP
#define FUTURES_EXECUTOR_DETAIL_EXECUTE_HPP

#include <futures/config.hpp>
#include <futures/executor/is_executor.hpp>
#include <futures/detail/deps/asio/execution/executor.hpp>
#include <futures/detail/deps/asio/is_executor.hpp>
#include <futures/detail/deps/boost/mp11/integral.hpp>
#include <type_traits>

namespace futures {
    namespace detail {
        template <class E, class F>
        void
        execute(E const& ex, F&& f);

        template <class E, class F>
        void
        execute(E& ex, F&& f);

        // futures executor
        template <class E, class F>
        void
        execute_impl(mp_int<0>, E const& ex, F&& f) {
            ex.execute(std::forward<F>(f));
        }

        // asio executor
        template <class E, class F>
        void
        execute_impl(mp_int<1>, E const& ex, F&& f) {
            ex.post(std::forward<F>(f), std::allocator<void>{});
        }

        // asio execution::executor
        template <class E, class F>
        void
        execute_impl(mp_int<2>, E const& ex, F&& f) {
            ex.execute(std::forward<F>(f));
        }

        // execution context
        template <class E, class F>
        void
        execute_impl(mp_int<3>, E& ex, F&& f) {
            execute(ex.get_executor(), std::forward<F>(f));
        }

        template <class E, class F>
        void
        execute(E const& ex, F&& f) {
            execute_impl(
                mp_cond<
                    asio::is_executor<E>,
                    mp_int<1>,
                    is_executor<E>,
                    mp_int<0>,
                    asio::execution::is_executor<E>,
                    mp_int<2>,
                    has_get_executor<E>,
                    mp_int<3>>{},
                ex,
                std::forward<F>(f));
        }

        template <class E, class F>
        void
        execute(E& ex, F&& f) {
            execute_impl(
                mp_cond<
                    asio::is_executor<E>,
                    mp_int<1>,
                    is_executor<E>,
                    mp_int<0>,
                    asio::execution::is_executor<E>,
                    mp_int<2>,
                    has_get_executor<E>,
                    mp_int<3>>{},
                ex,
                std::forward<F>(f));
        }
    } // namespace detail
} // namespace futures

#endif // FUTURES_EXECUTOR_DETAIL_EXECUTE_HPP
