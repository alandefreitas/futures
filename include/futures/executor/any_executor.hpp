//
// Copyright (c) 2023 alandefreitas (alandefreitas@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//

#ifndef FUTURES_EXECUTOR_ANY_EXECUTOR_HPP
#define FUTURES_EXECUTOR_ANY_EXECUTOR_HPP

/**
 *  @file executor/any_executor.hpp
 *  @brief Any executor
 *
 *  This file defines the any executor, which wraps and typeany erases other
 *  executors.
 */

#include <futures/config.hpp>
#include <futures/executor/inline_executor.hpp>
#include <futures/executor/is_executor.hpp>
#include <futures/detail/traits/std_type_traits.hpp>
#include <futures/detail/utility/sbo_ptr.hpp>
#include <futures/executor/detail/any_executor.hpp>

namespace futures {
    /** @addtogroup executors Executors
     *  @{
     */

    /// An wrapper that type erases any non-heterogeneous executor
    /**
     * The wrapped executor should be copy-constructible.
     */
    class any_executor {
        detail::sbo_ptr<detail::executor_interface> impl_;

    public:
        /// Destructor.
        ~any_executor() = default;

        /// Default constructor.
        /**
         * The wrapper will hold no executor.
         *
         * Any task sent for execution will be execute inline.
         */
        any_executor() noexcept = default;

        /// Copy constructor.
        any_executor(any_executor const& e) = default;

        /// Move constructor.
        any_executor(any_executor&& e) noexcept = default;

        /// Construct a wrapper for the specified executor.
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
                int>
            = 0>
#endif
        any_executor(E e);

        /// Assignment operator
        any_executor&
        operator=(any_executor const& e)
            = default;

        /// Move assignment operator.
        any_executor&
        operator=(any_executor&& e) noexcept
            = default;

        /// Assignment operator that sets the polymorphic wrapper to the empty
        /// state.
        any_executor&
        operator=(std::nullptr_t) {
            impl_.reset();
            return *this;
        }

        /// Construct a wrapper for the specified executor.
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
                int>
            = 0>
#endif
        any_executor&
        operator=(E e);

        /// Execute the function on the target executor.
        template <class F>
        void
        execute(F&& f) const;
    };

    /** @} */ // @addtogroup executors Executors
} // namespace futures

#include <futures/executor/impl/any_executor.hpp>

#endif // FUTURES_EXECUTOR_ANY_EXECUTOR_HPP
