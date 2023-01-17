//
// Copyright (c) 2022 alandefreitas (alandefreitas@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//

#ifndef FUTURES_EXECUTOR_IMPL_ANY_EXECUTOR_HPP
#define FUTURES_EXECUTOR_IMPL_ANY_EXECUTOR_HPP

#include <futures/executor/inline_executor.hpp>
#include <futures/detail/utility/invoke.hpp>

namespace futures {
#ifdef FUTURES_HAS_CONCEPTS
    template <executor E>
    requires(
        !std::same_as<any_executor, E> && std::copy_constructible<E>
        && std::is_nothrow_move_constructible_v<E>)
#else
    template <
        class E,
        std::enable_if_t<
            detail::conjunction_v<
                detail::mp_not<std::is_same<std::decay_t<E>, any_executor>>,
                is_executor<std::decay_t<E>>,
                std::is_copy_constructible<std::decay_t<E>>,
                std::is_nothrow_move_constructible<std::decay_t<E>>>,
            int>>
#endif
    any_executor::any_executor(E e)
        : impl_(detail::executor_interface_impl<E>(std::move(e))) {
    }

#ifdef FUTURES_HAS_CONCEPTS
    template <executor E>
    requires(
        !std::same_as<any_executor, E> && std::copy_constructible<E>
        && std::is_nothrow_move_constructible_v<E>)
#else
    template <
        class E,
        std::enable_if_t<
            is_executor_v<std::decay_t<E>>
                && detail::is_copy_constructible_v<std::decay_t<E>>
                && detail::is_nothrow_move_constructible_v<std::decay_t<E>>,
            int>>
#endif
    any_executor& any_executor::operator=(E e) {
        impl_ = std::move(detail::executor_interface_impl<E>(std::move(e)));
        return *this;
    }

    template <class F>
    void
    any_executor::execute(F&& f) const {
        if (impl_) {
            impl_->execute(std::forward<F>(f));
        } else {
            detail::invoke(std::forward<F>(f));
        }
    }
} // namespace futures

#endif // FUTURES_EXECUTOR_IMPL_ANY_EXECUTOR_HPP
