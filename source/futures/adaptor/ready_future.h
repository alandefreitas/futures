//
// Created by Alan Freitas on 8/18/21.
//

#ifndef FUTURES_READY_FUTURE_H
#define FUTURES_READY_FUTURE_H

#include <future>

#include <futures/futures/detail/traits/has_is_ready.h>

#include <futures/futures/traits/future_return.h>
#include <futures/futures/traits/is_future.h>

#include <futures/futures/basic_future.h>
#include <futures/futures/promise.h>

namespace futures {
    /** \addtogroup adaptors Adaptors
     *  @{
     */

    /// \brief Check if a future is ready
    /// Although basic_future has its more efficient is_ready function, this free function
    /// allows us to query other futures that don't implement is_ready, such as std::future.
    template <typename Future
#ifndef FUTURES_DOXYGEN
              ,
              std::enable_if_t<is_future_v<std::decay_t<Future>>, int> = 0
#endif
              >
    bool is_ready(Future &&f) {
        assert(f.valid() && "Undefined behaviour. Checking if an invalid future is ready.");
        if constexpr (detail::has_is_ready_v<Future>) {
            return f.is_ready();
        } else {
            return f.wait_for(std::chrono::seconds(0)) == std::future_status::ready;
        }
    }

    /// \brief Make a placeholder future object that is ready
    ///
    /// \see https://en.cppreference.com/w/cpp/experimental/make_ready_future
    ///
    /// \return A future associated with the shared state that is created.
    template <typename T, typename Future = future<typename std::decay_t<T>>> Future make_ready_future(T &&value) {
        using decay_type = typename std::decay_t<T>;
        promise<decay_type> p;
        Future result = p.template get_future<Future>();
        p.set_value(value);
        return result;
    }

    /// \brief Make a placeholder future object that is ready from a reference
    ///
    /// \see https://en.cppreference.com/w/cpp/experimental/make_ready_future
    ///
    /// \return A future associated with the shared state that is created.
    template <typename T, typename Future = future<T &>> Future make_ready_future(std::reference_wrapper<T> value) {
        promise<T &> p;
        Future result = p.template get_future<Future>();
        p.set_value(value);
        return result;
    }

    /// \brief Make a placeholder void future object that is ready
    ///
    /// \see https://en.cppreference.com/w/cpp/experimental/make_ready_future
    ///
    /// \return A future associated with the shared state that is created.
    template <typename Future = future<void>> Future make_ready_future() {
        promise<void> p;
        auto result = p.get_future<Future>();
        p.set_value();
        return result;
    }

    /// \brief Make a placeholder @ref cfuture object that is ready
    ///
    /// \see https://en.cppreference.com/w/cpp/experimental/make_ready_future
    ///
    /// \return A cfuture associated with the shared state that is created.
    template <typename T> cfuture<typename std::decay<T>> make_ready_cfuture(T &&value) {
        return make_ready_future<T, cfuture<typename std::decay<T>>>(std::forward<T>(value));
    }

    /// \brief Make a placeholder @ref cfuture object that is ready
    template <typename T> cfuture<T &> make_ready_cfuture(std::reference_wrapper<T> value) {
        return make_ready_future<T, cfuture<T &>>(value);
    }

    /// \brief Make a placeholder void @ref cfuture object that is ready
    ///
    /// \see https://en.cppreference.com/w/cpp/experimental/make_ready_future
    ///
    /// \return A cfuture associated with the shared state that is created.
    inline cfuture<void> make_ready_cfuture() { return make_ready_future<cfuture<void>>(); }

    /// \brief Make a placeholder @ref jcfuture object that is ready
    ///
    /// \see https://en.cppreference.com/w/cpp/experimental/make_ready_future
    ///
    /// \return A cfuture associated with the shared state that is created.
    template <typename T> jcfuture<typename std::decay<T>> make_ready_jcfuture(T &&value) {
        return make_ready_future<T, jcfuture<typename std::decay<T>>>(std::forward<T>(value));
    }

    /// \brief Make a placeholder @ref cfuture object that is ready
    template <typename T> jcfuture<T &> make_ready_jcfuture(std::reference_wrapper<T> value) {
        return make_ready_future<T, jcfuture<T &>>(value);
    }

    /// \brief Make a placeholder void @ref jcfuture object that is ready
    ///
    /// \see https://en.cppreference.com/w/cpp/experimental/make_ready_future
    ///
    /// \return A cfuture associated with the shared state that is created.
    inline jcfuture<void> make_ready_jcfuture() { return make_ready_future<jcfuture<void>>(); }

    /// \brief Make a placeholder future object that is ready with an exception from an exception ptr
    ///
    /// \see https://en.cppreference.com/w/cpp/experimental/make_exceptional_future
    ///
    /// \return A future associated with the shared state that is created.
    template <typename T, typename Future = future<T>> future<T> make_exceptional_future(std::exception_ptr ex) {
        promise<T> p;
        p.set_exception(ex);
        return p.template get_future<Future>();
    }

    /// \brief Make a placeholder future object that is ready with from any exception
    ///
    /// \see https://en.cppreference.com/w/cpp/experimental/make_exceptional_future
    ///
    /// \return A future associated with the shared state that is created.
    template <class T, typename Future = future<T>, class E> future<T> make_exceptional_future(E ex) {
        promise<T> p;
        p.set_exception(std::make_exception_ptr(ex));
        return p.template get_future<Future>();
    }
    /** @} */
} // namespace futures

#endif // FUTURES_READY_FUTURE_H
