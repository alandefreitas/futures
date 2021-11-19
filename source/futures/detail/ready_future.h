//
// Created by Alan Freitas on 8/18/21.
//

#ifndef FUTURES_READY_FUTURE_H
#define FUTURES_READY_FUTURE_H

#include <future>

#include "basic_future.h"
#include "traits/future_return.h"
#include "traits/is_future.h"

namespace futures {
    /** \addtogroup futures Futures
     *  @{
     */

    /** \addtogroup adaptors Adaptors
     *  @{
     */

    /// \brief Check if a future is ready
    template <typename Future, std::enable_if_t<is_future_v<Future>, int> = 0> bool is_ready(Future &&f) {
        assert(f.valid() && "Undefined behaviour. Checking if an invalid future is ready.");
        return f.wait_for(std::chrono::seconds(0)) == std::future_status::ready;
    }

    /// \brief Make a placeholder std::future object that is ready
    ///
    /// \see https://en.cppreference.com/w/cpp/experimental/make_ready_future
    ///
    /// \return A future associated with the shared state that is created.
    template <typename T> std::future<typename std::decay_t<T>> make_ready_future(T &&value) {
        using decay_type = typename std::decay_t<T>;
        std::promise<decay_type> p;
        std::future<decay_type> result = p.get_future();
        p.set_value(value);
        return result;
    }

    /// \brief Make a placeholder std::future object that is ready from a reference
    ///
    /// \see https://en.cppreference.com/w/cpp/experimental/make_ready_future
    ///
    /// \return A future associated with the shared state that is created.
    template <typename T> std::future<T &> make_ready_future(std::reference_wrapper<T> value) {
        std::promise<T &> p;
        std::future<T &> result = p.get_future();
        p.set_value(value);
        return result;
    }

    /// \brief Make a placeholder void std::future object that is ready
    ///
    /// \see https://en.cppreference.com/w/cpp/experimental/make_ready_future
    ///
    /// \return A future associated with the shared state that is created.
    inline std::future<void> make_ready_future() {
        std::promise<void> p;
        std::future<void> result = p.get_future();
        p.set_value();
        return result;
    }

    namespace detail {
        template <typename T> cfuture<typename std::decay_t<T>> internal_make_ready_cfuture(T &&value) {
            auto std_future = futures::make_ready_future(std::forward<T>(value));
            cfuture<typename std::decay_t<T>> c_future;
            c_future.set_future(std::move(std_future));
            return c_future;
        }

        template <typename T> cfuture<T &> internal_make_ready_cfuture(std::reference_wrapper<T> value) {
            auto std_future = futures::make_ready_future(std::forward<T>(value));
            cfuture<T&> c_future;
            c_future.set_future(std::move(std_future));
            return c_future;
        }

        inline cfuture<void> internal_make_ready_cfuture() {
            auto std_future = futures::make_ready_future();
            cfuture<void> c_future;
            c_future.set_future(std::make_unique<decltype(std_future)>(std::move(std_future)));
            return c_future;
        }
    } // namespace detail

    /// \brief Make a placeholder @ref cfuture object that is ready
    ///
    /// \see https://en.cppreference.com/w/cpp/experimental/make_ready_future
    ///
    /// \return A cfuture associated with the shared state that is created.
    template <typename T> decltype(auto) make_ready_cfuture(T &&value) {
        return detail::internal_make_ready_cfuture(std::forward<T>(value));
    }

    /// \brief Make a placeholder void @ref cfuture object that is ready
    ///
    /// \see https://en.cppreference.com/w/cpp/experimental/make_ready_future
    ///
    /// \return A cfuture associated with the shared state that is created.
    inline decltype(auto) make_ready_cfuture() {
        return detail::internal_make_ready_cfuture();
    }

    namespace detail {
        template <typename T> jcfuture<typename std::decay_t<T>> internal_make_ready_jcfuture(T &&value) {
            auto std_future = futures::make_ready_future(std::forward<T>(value));
            jcfuture<typename std::decay_t<T>> c_future;
            c_future.set_future(std::move(std_future));
            return c_future;
        }

        template <typename T> jcfuture<T &> internal_make_ready_jcfuture(std::reference_wrapper<T> value) {
            auto std_future = futures::make_ready_future(std::forward<T>(value));
            jcfuture<T&> c_future;
            c_future.set_future(std::move(std_future));
            return c_future;
        }

        inline jcfuture<void> internal_make_ready_jcfuture() {
            auto std_future = futures::make_ready_future();
            jcfuture<void> c_future;
            c_future.set_future(std::make_unique<decltype(std_future)>(std::move(std_future)));
            return c_future;
        }
    } // namespace detail

    /// \brief Make a placeholder @ref jcfuture object that is ready
    ///
    /// \see https://en.cppreference.com/w/cpp/experimental/make_ready_future
    ///
    /// \return A cfuture associated with the shared state that is created.
    template <typename T> decltype(auto) make_ready_jcfuture(T &&value) {
        return detail::internal_make_ready_jcfuture(std::forward<T>(value));
    }

    /// \brief Make a placeholder void @ref jcfuture object that is ready
    ///
    /// \see https://en.cppreference.com/w/cpp/experimental/make_ready_future
    ///
    /// \return A cfuture associated with the shared state that is created.
    inline decltype(auto) make_ready_jcfuture() {
        return detail::internal_make_ready_jcfuture();
    }

    /// \brief Make a placeholder future object that is ready with an exception from an exception ptr
    ///
    /// \see https://en.cppreference.com/w/cpp/experimental/make_exceptional_future
    ///
    /// \return A future associated with the shared state that is created.
    template <typename T> std::future<T> make_exceptional_future(std::exception_ptr ex) {
        std::promise<T> p;
        p.set_exception(ex);
        return p.get_future();
    }

    /// \brief Make a placeholder future object that is ready with from any exception
    ///
    /// \see https://en.cppreference.com/w/cpp/experimental/make_exceptional_future
    ///
    /// \return A future associated with the shared state that is created.
    template <class T, class E> std::future<T> make_exceptional_future(E ex) {
        std::promise<T> p;
        p.set_exception(std::make_exception_ptr(ex));
        return p.get_future();
    }
    /** @} */  // \addtogroup adaptors Adaptors
    /** @} */  // \addtogroup futures Futures
} // namespace futures

#endif // FUTURES_READY_FUTURE_H
