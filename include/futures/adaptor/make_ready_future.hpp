//
// Copyright (c) 2021 alandefreitas (alandefreitas@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//

#ifndef FUTURES_ADAPTOR_MAKE_READY_FUTURE_HPP
#define FUTURES_ADAPTOR_MAKE_READY_FUTURE_HPP

/**
 *  @file adaptor/make_ready_future.hpp
 *  @brief Make future object with value
 *
 *  This file defines the functions to create a future object whose value
 *  is already set.
 */

#include <futures/adaptor/detail/make_ready_future.hpp>

namespace futures {
    /** @addtogroup adaptors Adaptors
     *  @{
     */

    /// Make a placeholder future object that is ready at construction
    /**
     *  @param value The value for the new future
     *  @return A future associated with the shared state that is created.
     *
     *  @see https://en.cppreference.com/w/cpp/experimental/make_ready_future
     */
    template <class T>
    basic_future<typename std::decay_t<T>, future_options<>>
    make_ready_future(T &&value) {
        return detail::make_ready_future_impl{}.template make_ready_future<T>(
            std::forward<T>(value));
    }

    /// @copydoc make_ready_future
    template <class T>
    basic_future<T &, future_options<>>
    make_ready_future(std::reference_wrapper<T> value) {
        return detail::make_ready_future_impl{}.template make_ready_future<T>(
            value);
    }

    /// @copydoc make_ready_future
    inline basic_future<void, future_options<>>
    make_ready_future() {
        return detail::make_ready_future_impl{}.make_ready_future();
    }

    /// Make a placeholder future object that is ready with an exception
    /// from an exception ptr
    /**
     *  @param ex The exception for the new future
     *  @return A future associated with the shared state that is created.
     *
     *  @see
     *  https://en.cppreference.com/w/cpp/experimental/make_exceptional_future
     *
     */
    template <class T = void>
    basic_future<T, future_options<>>
    make_exceptional_future(std::exception_ptr ex) {
        return detail::make_ready_future_impl{}
            .template make_exceptional_future<T>(ex);
    }

    /// @copydoc make_exceptional_future
    template <class T = void, class E>
    basic_future<T, future_options<>>
    make_exceptional_future(E ex) {
        return detail::make_ready_future_impl{}
            .template make_exceptional_future<T, E>(ex);
    }

    /** @} */
} // namespace futures

#endif // FUTURES_ADAPTOR_MAKE_READY_FUTURE_HPP
