//
// Copyright (c) 2021 alandefreitas (alandefreitas@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//

#ifndef FUTURES_EXECUTOR_INLINE_EXECUTOR_HPP
#define FUTURES_EXECUTOR_INLINE_EXECUTOR_HPP

#include <futures/detail/config/asio_include.hpp>
#include <futures/executor/is_executor.hpp>

namespace futures {
    /** \addtogroup executors Executors
     *  @{
     */

    /// \brief A minimal executor that runs anything in the local thread in the
    /// default context
    ///
    /// Although simple, it needs to meet the executor requirements:
    /// - Executor concept
    /// - Ability to query the execution context
    ///     - Result being derived from execution_context
    /// - The execute function
    /// \see https://think-async.com/Asio/asio-1.18.2/doc/asio/std_executors.html
    struct inline_executor
    {
        asio::execution_context *context_{ nullptr };

        constexpr bool
        operator==(const inline_executor &other) const noexcept {
            return context_ == other.context_;
        }

        constexpr bool
        operator!=(const inline_executor &other) const noexcept {
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
            f();
        }
    };

    /// \brief Get the inline execution context
    inline asio::execution_context &
    inline_execution_context() {
        static asio::execution_context context;
        return context;
    }

    /// \brief Make an inline executor object
    inline inline_executor
    make_inline_executor() {
        asio::execution_context &ctx = inline_execution_context();
        return inline_executor{ &ctx };
    }

    /** @} */ // \addtogroup executors Executors
} // namespace futures

#ifdef FUTURES_USE_BOOST_ASIO
namespace boost {
#endif
    namespace asio {
        /// \brief Ensure asio and our internal functions see inline_executor as
        /// an executor
        ///
        /// This traits ensures asio and our internal functions see
        /// inline_executor as an executor, as asio traits don't always work.
        ///
        /// This is quite a workaround until things don't improve with our
        /// executor traits.
        ///
        /// Ideally, we would have our own executor traits and let asio pick up
        /// from those.
        ///
        template <>
        class is_executor<futures::inline_executor> : public std::true_type
        {};

        namespace traits {
#if !defined(ASIO_HAS_DEDUCED_EXECUTE_MEMBER_TRAIT)
            template <typename F>
            struct execute_member<futures::inline_executor, F>
            {
                static constexpr bool is_valid = true;
                static constexpr bool is_noexcept = true;
                typedef void result_type;
            };

#endif // !defined(ASIO_HAS_DEDUCED_EXECUTE_MEMBER_TRAIT)
#if !defined(ASIO_HAS_DEDUCED_EQUALITY_COMPARABLE_TRAIT)
            template <>
            struct equality_comparable<futures::inline_executor>
            {
                static constexpr bool is_valid = true;
                static constexpr bool is_noexcept = true;
            };

#endif // !defined(ASIO_HAS_DEDUCED_EQUALITY_COMPARABLE_TRAIT)
#if !defined(ASIO_HAS_DEDUCED_QUERY_MEMBER_TRAIT)
            template <>
            struct query_member<
                futures::inline_executor,
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
                futures::inline_executor,
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

#endif // FUTURES_EXECUTOR_INLINE_EXECUTOR_HPP
