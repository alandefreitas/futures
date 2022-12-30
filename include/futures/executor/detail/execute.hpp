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
#include <futures/detail/deps/boost/mp11/integral.hpp>
#include <type_traits>

namespace futures {
    namespace detail {
        // futures executor or asio::execution::executor
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

        template <class E, class F>
        void
        execute_in_executor(E const& ex, F&& f) {
            detail::execute_impl(
                detail::mp_cond<
                    detail::is_asio_executor<E>,
                    detail::mp_int<1>,
                    is_executor<E>,
                    detail::mp_int<0>>{},
                ex,
                std::forward<F>(f));
        }

        // execution context
        template <class E, class F>
        void
        execute_in_context(E& ex, F&& f) {
            execute_in_executor(ex.get_executor(), std::forward<F>(f));
        }
    } // namespace detail
} // namespace futures

#endif // FUTURES_EXECUTOR_DETAIL_EXECUTE_HPP
