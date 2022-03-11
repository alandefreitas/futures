//
// Copyright (c) 2022 alandefreitas (alandefreitas@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//

#ifndef FUTURES_DETAIL_UTILITY_MAYBE_ATOMIC_HPP
#define FUTURES_DETAIL_UTILITY_MAYBE_ATOMIC_HPP

#include <atomic>

namespace futures::detail {

    /// A class that holds an element of type T in a conditionally atomic way
    /**
     * Depending on the type of the operation state, in particular whether its
     * task is deferred, the synchronization primitives don't really need
     * atomic operations.
     *
     * This class encapsulates this logic so that real atomic operations can be
     * conditionally disabled. Only the operations necessary to atomic
     * operations are implemented by this class.
     *
     * @tparam T Value type
     * @tparam Enable Whether atomic operations should be enabled
     */
    template <class T, bool Enable>
    class maybe_atomic;

    template <class T>
    class maybe_atomic<T, true>
    {
    public:
        /// Initializes the underlying object with desired.
        /**
         *  The initialization is not atomic.
         */
        constexpr maybe_atomic(T desired) noexcept : value_(desired) {}

        /// Atomically obtains the value of the atomic object
        /**
         * Atomically loads and returns the current value of the atomic
         * variable.
         *
         * Memory is affected according to the value of order.
         *
         * @param order memory order constraints to enforce
         *
         * @return The current value of the atomic variable
         */
        T
        load(std::memory_order order = std::memory_order_seq_cst)
            const noexcept {
            return value_.load(order);
        }


        /// Atomically replaces the underlying value with desired
        /**
         * The operation is read-modify-write operation.
         *
         * Memory is affected according to the value of order.
         *
         * @param desired value to assign
         * @param order memory order constraints to enforce
         *
         * @return The value of the atomic variable before the call.
         */
        T
        exchange(
            T desired,
            std::memory_order order = std::memory_order_seq_cst) noexcept {
            return value_.exchange(desired, order);
        }

        /// Atomically compares the object representation of *this with that of
        /// expected, and if those are bitwise-equal, replaces the former with
        /// desired
        /**
         * In the (2) and (4) versions order is used for both read-modify-write
         * and load operations, except that std::memory_order_acquire and
         * std::memory_order_relaxed are used for the load operation if
         * order == std::memory_order_acq_rel, or
         * order == std::memory_order_release respectively.
         *
         * @param expected value expected to be found in the atomic object.
         * @param desired value to store in the object if it is as expected
         * @param order the memory synchronization ordering for both operations
         *
         * @return true if the underlying atomic value was successfully changed,
         * false otherwise
         */
        bool
        compare_exchange_strong(
            T& expected,
            T desired,
            std::memory_order order = std::memory_order_seq_cst) noexcept {
            return value_.compare_exchange_strong(expected, desired, order);
        }

    private:
        std::atomic<T> value_;
    };

    template <class T>
    class maybe_atomic<T, false>
    {
    public:
        /// Initializes the underlying object with desired.
        /**
         *  The initialization is not atomic.
         */
        constexpr maybe_atomic(T desired) noexcept : value_(desired) {}

        /// Atomically obtains the value of the atomic object
        /**
         * Atomically loads and returns the current value of the atomic
         * variable.
         *
         * Memory is affected according to the value of order.
         *
         * @param order memory order constraints to enforce
         *
         * @return The current value of the atomic variable
         */
        T
        load(std::memory_order order = std::memory_order_seq_cst)
            const noexcept {
            (void) order;
            return value_;
        }

        /// Atomically replaces the underlying value with desired
        /**
         * The operation is read-modify-write operation.
         *
         * Memory is affected according to the value of order.
         *
         * @param desired value to assign
         * @param order memory order constraints to enforce
         *
         * @return The value of the atomic variable before the call.
         */
        T
        exchange(
            T desired,
            std::memory_order order = std::memory_order_seq_cst) noexcept {
            (void) order;
            return std::exchange(value_, desired);
        }

        /// Atomically compares the object representation of *this with that of
        /// expected, and if those are bitwise-equal, replaces the former with
        /// desired
        /**
         * In the (2) and (4) versions order is used for both read-modify-write
         * and load operations, except that std::memory_order_acquire and
         * std::memory_order_relaxed are used for the load operation if
         * order == std::memory_order_acq_rel, or
         * order == std::memory_order_release respectively.
         *
         * @param expected value expected to be found in the atomic object.
         * @param desired value to store in the object if it is as expected
         * @param order the memory synchronization ordering for both operations
         *
         * @return true if the underlying atomic value was successfully changed,
         * false otherwise
         */
        bool
        compare_exchange_strong(
            T& expected,
            T desired,
            std::memory_order order = std::memory_order_seq_cst) noexcept {
            (void) order;
            bool match_expected = value_ == expected;
            expected = value_;
            if (match_expected) {
                value_ = desired;
                return true;
            }
            return false;
        }

    private:
        T value_;
    };

    /// Establishes memory synchronization ordering of non-atomic and relaxed
    /// atomic accesses, as instructed by order, without an associated atomic
    /// operation.
    /**
     *  @tparam Enable Whether the synchronization should be enabled
     *  @param order The memory order
     */
    template <bool Enable>
    inline void
    maybe_atomic_thread_fence(std::memory_order order) noexcept;

    template <>
    inline void
    maybe_atomic_thread_fence<true>(std::memory_order order) noexcept {
        std::atomic_thread_fence(order);
    }

    template <>
    inline void
    maybe_atomic_thread_fence<false>(std::memory_order order) noexcept {
        (void) order;
    }

} // namespace futures::detail

#endif // FUTURES_DETAIL_UTILITY_MAYBE_ATOMIC_HPP
