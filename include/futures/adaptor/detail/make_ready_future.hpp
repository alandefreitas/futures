//
// Copyright (c) 2021 alandefreitas (alandefreitas@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//

#ifndef FUTURES_ADAPTOR_DETAIL_MAKE_READY_FUTURE_HPP
#define FUTURES_ADAPTOR_DETAIL_MAKE_READY_FUTURE_HPP

#include <futures/basic_future.hpp>
#include <futures/promise.hpp>
#include <futures/traits/is_future.hpp>
#include <future>

namespace futures::detail {
    struct make_ready_future_impl {
        template <typename T>
        basic_future<typename std::decay_t<T>, future_options<>>
        make_ready_future(T &&value) {
            basic_future<std::decay_t<T>, future_options<>> result(
                std::forward<T>(value));
            return result;
        }

        template <typename T>
        basic_future<T &, future_options<>>
        make_ready_future(std::reference_wrapper<T> value) {
            promise<T &, future_options<>> p;
            basic_future<T &, future_options<>> result = p.get_future();
            p.set_value(value);
            return result;
        }

        basic_future<void, future_options<>>
        make_ready_future() {
            promise<void, future_options<>> p;
            basic_future<void, future_options<>> result = p.get_future();
            p.set_value();
            return result;
        }

        template <typename T = void>
        basic_future<T, future_options<>>
        make_exceptional_future(std::exception_ptr ex) {
            promise<T, future_options<>> p;
            p.set_exception(ex);
            return p.get_future();
        }

        template <class T = void, class E>
        basic_future<T, future_options<>>
        make_exceptional_future(E ex) {
            promise<T, future_options<>> p;
            p.set_exception(std::make_exception_ptr(ex));
            return p.get_future();
        }
    };
} // namespace futures::detail

#endif // FUTURES_ADAPTOR_DETAIL_MAKE_READY_FUTURE_HPP
