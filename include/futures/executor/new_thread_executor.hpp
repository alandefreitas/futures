//
// Copyright (c) 2021 alandefreitas (alandefreitas@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//

#ifndef FUTURES_EXECUTOR_NEW_THREAD_EXECUTOR_HPP
#define FUTURES_EXECUTOR_NEW_THREAD_EXECUTOR_HPP

/**
 *  @file executor/new_thread_executor.hpp
 *  @brief New thread executor
 *
 *  This file defines the new thread executor, which creates a new thread
 *  every time a new task is launched. This is somewhat equivalent to executing
 *  tasks with C++11 `std::async`.
 */

#include <futures/config.hpp>
#include <futures/launch.hpp>
#include <futures/executor/inline_executor.hpp>
#include <futures/executor/is_executor.hpp>
#include <futures/detail/deps/asio/execution.hpp>
#include <futures/detail/deps/asio/execution_context.hpp>

namespace futures {
    /** @addtogroup executors Executors
     *  @{
     */

    /// An executor that runs anything in a new thread, like std::async does
    struct new_thread_executor {
        asio::execution_context *context_{ nullptr };

        constexpr bool
        operator==(new_thread_executor const &other) const noexcept {
            return context_ == other.context_;
        }

        constexpr bool
        operator!=(new_thread_executor const &other) const noexcept {
            return !(*this == other);
        }

        [[nodiscard]] constexpr asio::execution_context &
        query(asio::execution::context_t) const noexcept {
            return *context_;
        }

        static constexpr asio::execution::blocking_t::never_t
        query(asio::execution::blocking_t) noexcept {
            return asio::execution::blocking_t::never;
        }

        template <class F>
        void
        execute(F f) const {
            auto fut = std::async(std::launch::async, f);
            fut.wait_for(std::chrono::seconds(0));
        }
    };

    /// Make an new thread executor object
    FUTURES_DECLARE new_thread_executor
    make_new_thread_executor();

    /** @} */ // @addtogroup executors Executors
} // namespace futures

#ifdef FUTURES_USE_BOOST_ASIO
namespace boost {
#endif
    namespace asio {
        /// Ensure asio (and our internal functions) sees these as
        /// executors, as traits don't always work
        /**
         * This is quite a workaround until things don't improve with our
         * executor traits.
         *
         * Ideally, we would have our own executor traits and let asio pick up
         * from those.
         **/
        template <>
        class is_executor<futures::new_thread_executor>
            : public std::true_type {};

        namespace traits {
#if !defined(ASIO_HAS_DEDUCED_EXECUTE_MEMBER_TRAIT)
            template <class F>
            struct execute_member<futures::new_thread_executor, F> {
                static constexpr bool is_valid = true;
                static constexpr bool is_noexcept = true;
                typedef void result_type;
            };
#endif // !defined(ASIO_HAS_DEDUCED_EXECUTE_MEMBER_TRAIT)
#if !defined(ASIO_HAS_DEDUCED_EQUALITY_COMPARABLE_TRAIT)
            template <>
            struct equality_comparable<futures::new_thread_executor> {
                static constexpr bool is_valid = true;
                static constexpr bool is_noexcept = true;
            };

#endif // !defined(ASIO_HAS_DEDUCED_EQUALITY_COMPARABLE_TRAIT)
#if !defined(ASIO_HAS_DEDUCED_QUERY_MEMBER_TRAIT)
            template <>
            struct query_member<
                futures::new_thread_executor,
                asio::execution::context_t> {
                static constexpr bool is_valid = true;
                static constexpr bool is_noexcept = true;
                typedef asio::execution_context &result_type;
            };

#endif // !defined(ASIO_HAS_DEDUCED_QUERY_MEMBER_TRAIT)
#if !defined(ASIO_HAS_DEDUCED_QUERY_STATIC_CONSTEXPR_MEMBER_TRAIT)
            template <class Property>
            struct query_static_constexpr_member<
                futures::new_thread_executor,
                Property,
                typename enable_if<std::is_convertible<
                    Property,
                    asio::execution::blocking_t>::value>::type> {
                static constexpr bool is_valid = true;
                static constexpr bool is_noexcept = true;
                typedef asio::execution::blocking_t::never_t result_type;
                static constexpr result_type
                value() noexcept {
                    return result_type();
                }
            };

#endif // !defined(ASIO_HAS_DEDUCED_QUERY_STATIC_CONSTEXPR_MEMBER_TRAIT)

        } // namespace traits
    }     // namespace asio
#ifdef FUTURES_USE_BOOST_ASIO
}
#endif

#if FUTURES_HEADER_ONLY
#    include <futures/executor/impl/new_thread_executor.hpp>
#endif

#endif // FUTURES_EXECUTOR_NEW_THREAD_EXECUTOR_HPP
