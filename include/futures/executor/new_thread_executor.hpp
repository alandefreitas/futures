//
// Copyright (c) 2021 alandefreitas (alandefreitas@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//

#ifndef FUTURES_NEW_THREAD_EXECUTOR_H
#define FUTURES_NEW_THREAD_EXECUTOR_H

#include <futures/config/asio_include.hpp>
#include <futures/executor/inline_executor.hpp>
#include <futures/executor/is_executor.hpp>

namespace futures {
    /** \addtogroup executors Executors
     *  @{
     */

    /// \brief An executor that runs anything in a new thread, like std::async
    /// does
    struct new_thread_executor
    {
        asio::execution_context *context_{ nullptr };

        constexpr bool
        operator==(const new_thread_executor &other) const noexcept {
            return context_ == other.context_;
        }

        constexpr bool
        operator!=(const new_thread_executor &other) const noexcept {
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

    /// \brief Make an new thread executor object
    new_thread_executor
    make_new_thread_executor() {
        asio::execution_context &ctx = inline_execution_context();
        return new_thread_executor{ &ctx };
    }
    /** @} */ // \addtogroup executors Executors
} // namespace futures

#ifdef FUTURES_USE_BOOST_ASIO
namespace boost {
#endif
    namespace asio {
        /// \brief Ensure asio (and our internal functions) sees these as
        /// executors, as traits don't always work
        ///
        /// This is quite a workaround until things don't improve with our
        /// executor traits.
        ///
        /// Ideally, we would have our own executor traits and let asio pick up
        /// from those.
        ///
        template <>
        class is_executor<futures::new_thread_executor> : public std::true_type
        {};

        namespace traits {
#if !defined(ASIO_HAS_DEDUCED_EXECUTE_MEMBER_TRAIT)
            template <typename F>
            struct execute_member<futures::new_thread_executor, F>
            {
                static constexpr bool is_valid = true;
                static constexpr bool is_noexcept = true;
                typedef void result_type;
            };
#endif // !defined(ASIO_HAS_DEDUCED_EXECUTE_MEMBER_TRAIT)
#if !defined(ASIO_HAS_DEDUCED_EQUALITY_COMPARABLE_TRAIT)
            template <>
            struct equality_comparable<futures::new_thread_executor>
            {
                static constexpr bool is_valid = true;
                static constexpr bool is_noexcept = true;
            };

#endif // !defined(ASIO_HAS_DEDUCED_EQUALITY_COMPARABLE_TRAIT)
#if !defined(ASIO_HAS_DEDUCED_QUERY_MEMBER_TRAIT)
            template <>
            struct query_member<
                futures::new_thread_executor,
                asio::execution::context_t>
            {
                static constexpr bool is_valid = true;
                static constexpr bool is_noexcept = true;
                typedef asio::execution_context &result_type;
            };

#endif // !defined(ASIO_HAS_DEDUCED_QUERY_MEMBER_TRAIT)
#if !defined(ASIO_HAS_DEDUCED_QUERY_STATIC_CONSTEXPR_MEMBER_TRAIT)
            template <typename Property>
            struct query_static_constexpr_member<
                futures::new_thread_executor,
                Property,
                typename enable_if<std::is_convertible<
                    Property,
                    asio::execution::blocking_t>::value>::type>
            {
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

#endif // FUTURES_NEW_THREAD_EXECUTOR_H
