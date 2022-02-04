//
// Copyright (c) 2021 alandefreitas (alandefreitas@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//

#ifndef FUTURES_ADAPTOR_READY_FUTURE_HPP
#define FUTURES_ADAPTOR_READY_FUTURE_HPP

#include <futures/futures/basic_future.hpp>
#include <futures/futures/promise.hpp>
#include <futures/futures/traits/future_return.hpp>
#include <futures/futures/traits/is_future.hpp>
#include <futures/detail/traits/has_is_ready.hpp>
#include <future>

namespace futures {
    /** \addtogroup adaptors Adaptors
     *  @{
     */

    /// \brief Check if a future is ready
    ///
    /// Although basic_future has its more efficient is_ready function, this
    /// free function allows us to query other futures that don't implement
    /// is_ready, such as std::future.
    template <
        typename Future
#ifndef FUTURES_DOXYGEN
        ,
        std::enable_if_t<is_future_v<std::decay_t<Future>>, int> = 0
#endif
        >
    bool
    is_ready(Future &&f) {
        assert(
            f.valid()
            && "Undefined behaviour. Checking if an invalid future is ready.");
        if constexpr (detail::has_is_ready_v<Future>) {
            return f.is_ready();
        } else {
            return f.wait_for(std::chrono::seconds(0))
                   == std::future_status::ready;
        }
    }

    /// \brief Make a placeholder future object that is ready
    ///
    /// \see https://en.cppreference.com/w/cpp/experimental/make_ready_future
    ///
    /// \return A future associated with the shared state that is created.
    template <typename T>
    basic_future<typename std::decay_t<T>, future_options<>>
    make_ready_future(T &&value) {
        promise<std::decay_t<T>, future_options<>> p;
        basic_future<std::decay_t<T>, future_options<>> result = p.get_future();
        p.set_value(std::forward<T>(value));
        return result;
    }

    /// \brief Make a placeholder future object that is ready from a reference
    ///
    /// \see https://en.cppreference.com/w/cpp/experimental/make_ready_future
    ///
    /// \return A future associated with the shared state that is created.
    template <typename T>
    basic_future<T &, future_options<>>
    make_ready_future(std::reference_wrapper<T> value) {
        promise<T &, future_options<>> p;
        basic_future<T &, future_options<>> result = p.get_future();
        p.set_value(value);
        return result;
    }

    /// \brief Make a placeholder void future object that is ready
    ///
    /// \see https://en.cppreference.com/w/cpp/experimental/make_ready_future
    ///
    /// \return A future associated with the shared state that is created.
    inline basic_future<void, future_options<>>
    make_ready_future() {
        promise<void, future_options<>> p;
        basic_future<void, future_options<>> result = p.get_future();
        p.set_value();
        return result;
    }

    /// \brief Make a placeholder future object that is ready with an exception
    /// from an exception ptr
    ///
    /// \see
    /// https://en.cppreference.com/w/cpp/experimental/make_exceptional_future
    ///
    /// \return A future associated with the shared state that is created.
    template <typename T = void>
    basic_future<T, future_options<>>
    make_exceptional_future(std::exception_ptr ex) {
        promise<T, future_options<>> p;
        p.set_exception(ex);
        return p.get_future();
    }

    /// \brief Make a placeholder future object that is ready with from any
    /// exception
    ///
    /// \see
    /// https://en.cppreference.com/w/cpp/experimental/make_exceptional_future
    ///
    /// \return A future associated with the shared state that is created.
    template <class T = void, class E>
    basic_future<T, future_options<>>
    make_exceptional_future(E ex) {
        promise<T, future_options<>> p;
        p.set_exception(std::make_exception_ptr(ex));
        return p.get_future();
    }
    /** @} */
} // namespace futures

#endif // FUTURES_ADAPTOR_READY_FUTURE_HPP
