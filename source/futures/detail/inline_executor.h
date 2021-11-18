//
// Created by Alan Freitas on 8/17/21.
//

#ifndef FUTURES_INLINE_EXECUTOR_H
#define FUTURES_INLINE_EXECUTOR_H

// Don't let asio compile definitions at this point
#ifndef ASIO_SEPARATE_COMPILATION
#define ASIO_SEPARATE_COMPILATION
#endif
#include <futures/detail/asio_include.h>

namespace futures {
    /** \addtogroup Executors
     *  @{
     */

    /// \brief A minimal executor that runs anything in the local thread in the default context
    ///
    /// Although simple, it needs to meet the executor requirements:
    /// - Executor concept
    /// - Ability to query the execution context
    ///     - Result being derived from execution_context
    /// - The execute function
    /// \see https://think-async.com/Asio/asio-1.18.2/doc/asio/std_executors.html
    struct inline_executor {
        asio::execution_context *context_{nullptr};

        constexpr bool operator==(const inline_executor &other) const noexcept { return context_ == other.context_; }

        constexpr bool operator!=(const inline_executor &other) const noexcept { return !(*this == other); }

        [[nodiscard]] constexpr asio::execution_context &query(asio::execution::context_t) const noexcept {
            return *context_;
        }

        static constexpr asio::execution::blocking_t::never_t query(asio::execution::blocking_t) noexcept {
            return asio::execution::blocking_t::never;
        }

        template <class F> void execute(F f) const { f(); }
    };

    /// \brief Get the inline execution context
    asio::execution_context &inline_execution_context();

    /// \brief Make an inline executor object
    inline_executor make_inline_executor();

    /// \brief An executor that runs anything in a new thread, like std::async does
    struct new_thread_executor {
        asio::execution_context *context_{nullptr};

        constexpr bool operator==(const new_thread_executor &other) const noexcept {
            return context_ == other.context_;
        }

        constexpr bool operator!=(const new_thread_executor &other) const noexcept { return !(*this == other); }

        [[nodiscard]] constexpr asio::execution_context &query(asio::execution::context_t) const noexcept {
            return *context_;
        }

        static constexpr asio::execution::blocking_t::never_t query(asio::execution::blocking_t) noexcept {
            return asio::execution::blocking_t::never;
        }

        template <class F> void execute(F f) const {
            auto fut = std::async(std::launch::async, f);
            fut.wait_for(std::chrono::seconds(0));
        }
    };

    /// \brief Make an new thread executor object
    new_thread_executor make_new_thread_executor();

    /// \brief An executor that runs anything inline later when result is requested,
    /// like std::async with std::launch::defer does
    struct inline_later_executor {
        asio::execution_context *context_{nullptr};

        constexpr bool operator==(const inline_later_executor &other) const noexcept {
            return context_ == other.context_;
        }

        constexpr bool operator!=(const inline_later_executor &other) const noexcept { return !(*this == other); }

        [[nodiscard]] constexpr asio::execution_context &query(asio::execution::context_t) const noexcept {
            return *context_;
        }

        static constexpr asio::execution::blocking_t::never_t query(asio::execution::blocking_t) noexcept {
            return asio::execution::blocking_t::never;
        }

        template <class F> void execute(F f) const {
            auto fut = std::async(std::launch::deferred, f);
            fut.wait_for(std::chrono::seconds(0));
        }
    };

    /// \brief Make an new thread executor object
    inline_later_executor make_inline_later_executor();
    /** @} */  // \addtogroup Executors
} // namespace futures

namespace asio {
    /// \brief Ensure asio (and our internal functions) sees these as executors, as traits don't always work
    template <> class is_executor<futures::inline_executor> : public std::true_type {};
    template <> class is_executor<futures::inline_later_executor> : public std::true_type {};
    template <> class is_executor<futures::new_thread_executor> : public std::true_type {};

    namespace traits {
#if !defined(ASIO_HAS_DEDUCED_EXECUTE_MEMBER_TRAIT)
        template <typename F> struct execute_member<futures::inline_executor, F> {
            static constexpr bool is_valid = true;
            static constexpr bool is_noexcept = true;
            typedef void result_type;
        };

        template <typename F> struct execute_member<futures::inline_later_executor, F> {
            static constexpr bool is_valid = true;
            static constexpr bool is_noexcept = true;
            typedef void result_type;
        };

        template <typename F> struct execute_member<futures::new_thread_executor, F> {
            static constexpr bool is_valid = true;
            static constexpr bool is_noexcept = true;
            typedef void result_type;
        };
#endif // !defined(ASIO_HAS_DEDUCED_EXECUTE_MEMBER_TRAIT)
#if !defined(ASIO_HAS_DEDUCED_EQUALITY_COMPARABLE_TRAIT)
        template <> struct equality_comparable<futures::inline_executor> {
            static constexpr bool is_valid = true;
            static constexpr bool is_noexcept = true;
        };

        template <> struct equality_comparable<futures::inline_later_executor> {
            static constexpr bool is_valid = true;
            static constexpr bool is_noexcept = true;
        };

        template <> struct equality_comparable<futures::new_thread_executor> {
            static constexpr bool is_valid = true;
            static constexpr bool is_noexcept = true;
        };

#endif // !defined(ASIO_HAS_DEDUCED_EQUALITY_COMPARABLE_TRAIT)
#if !defined(ASIO_HAS_DEDUCED_QUERY_MEMBER_TRAIT)
        template <> struct query_member<futures::inline_executor, asio::execution::context_t> {
            static constexpr bool is_valid = true;
            static constexpr bool is_noexcept = true;
            typedef asio::execution_context &result_type;
        };

        template <> struct query_member<futures::inline_later_executor, asio::execution::context_t> {
            static constexpr bool is_valid = true;
            static constexpr bool is_noexcept = true;
            typedef asio::execution_context &result_type;
        };

        template <> struct query_member<futures::new_thread_executor, asio::execution::context_t> {
            static constexpr bool is_valid = true;
            static constexpr bool is_noexcept = true;
            typedef asio::execution_context &result_type;
        };

#endif // !defined(ASIO_HAS_DEDUCED_QUERY_MEMBER_TRAIT)
#if !defined(ASIO_HAS_DEDUCED_QUERY_STATIC_CONSTEXPR_MEMBER_TRAIT)
        template <typename Property>
        struct query_static_constexpr_member<
            futures::inline_executor, Property,
            typename enable_if<std::is_convertible<Property, asio::execution::blocking_t>::value>::type> {
            static constexpr bool is_valid = true;
            static constexpr bool is_noexcept = true;
            typedef asio::execution::blocking_t::never_t result_type;
            static constexpr result_type value() noexcept { return result_type(); }
        };

        template <typename Property>
        struct query_static_constexpr_member<
            futures::inline_later_executor, Property,
            typename enable_if<std::is_convertible<Property, asio::execution::blocking_t>::value>::type> {
            static constexpr bool is_valid = true;
            static constexpr bool is_noexcept = true;
            typedef asio::execution::blocking_t::never_t result_type;
            static constexpr result_type value() noexcept { return result_type(); }
        };

        template <typename Property>
        struct query_static_constexpr_member<
            futures::new_thread_executor, Property,
            typename enable_if<std::is_convertible<Property, asio::execution::blocking_t>::value>::type> {
            static constexpr bool is_valid = true;
            static constexpr bool is_noexcept = true;
            typedef asio::execution::blocking_t::never_t result_type;
            static constexpr result_type value() noexcept { return result_type(); }
        };

#endif // !defined(ASIO_HAS_DEDUCED_QUERY_STATIC_CONSTEXPR_MEMBER_TRAIT)

    } // namespace traits
} // namespace asio

#endif // FUTURES_INLINE_EXECUTOR_H
