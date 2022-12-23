//
// Copyright (c) 2021 alandefreitas (alandefreitas@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//

#ifndef FUTURES_DETAIL_THREAD_LOCK_HPP
#define FUTURES_DETAIL_THREAD_LOCK_HPP

#include <futures/algorithm/traits/is_input_iterator.hpp>

namespace futures {
    namespace detail {
        /** @addtogroup futures Futures
         *  @{
         */

        /// Try to lock range of mutexes in a way that all of them should
        /// work
        ///
        /// Calls try_lock() on each of the Lockable objects in the supplied
        /// range. If any of the calls to try_lock() returns false then all
        /// locks acquired are released and an iterator referencing the failed
        /// lock is returned.
        ///
        /// If any of the try_lock() operations on the supplied Lockable objects
        /// throws an exception any locks acquired by the function will be
        /// released before the function exits.
        ///
        /// @throws exception Any exceptions thrown by calling try_lock() on the
        /// supplied Lockable objects
        ///
        /// @post All the supplied Lockable objects are locked by the calling
        /// thread.
        ///
        /// @see
        /// https://www.boost.org/doc/libs/1_78_0/doc/html/thread/synchronization.html#thread.synchronization.lock_functions.try_lock_range
        ///
        /// @tparam Iterator Range iterator type
        /// @param first Iterator to first mutex in the range
        /// @param last Iterator to one past the last mutex in the range
        /// @return Iterator to first element that could *not* be locked, or
        /// `end` if all the supplied Lockable objects are now locked
        template <
            class Iterator,
            std::enable_if_t<is_input_iterator_v<Iterator>, int> = 0>
        Iterator
        try_lock(Iterator first, Iterator last) {
            using lock_type = typename std::iterator_traits<
                Iterator>::value_type;

            // Handle trivial cases
            if (first == last) {
                // empty_range
                return last;
            } else if (std::next(first) == last) {
                // single_element
                if (first->try_lock()) {
                    return last;
                } else {
                    return first;
                }
            }

            // General cases: Try to lock first and already return if fails
            std::unique_lock<lock_type> guard_first(*first, std::try_to_lock);
            if (!guard_first.owns_lock()) {
                // locking_failed
                return first;
            }

            // While first is locked by guard_first, try to lock the other
            // elements in the range
            const Iterator failed_mutex_it = try_lock(std::next(first), last);
            if (failed_mutex_it == last) {
                // none_failed
                // Break the association of the associated mutex (i.e. don't
                // unlock at destruction)
                guard_first.release();
            }
            return failed_mutex_it;
        }

        /// Lock range of mutexes in a way that avoids deadlock
        ///
        /// Locks the Lockable objects in the range [`first`, `last`) supplied
        /// as arguments in an unspecified and indeterminate order in a way that
        /// avoids deadlock. It is safe to call this function concurrently from
        /// multiple threads for any set of mutexes (or other lockable objects)
        /// in any order without risk of deadlock. If any of the lock() or
        /// try_lock() operations on the supplied Lockable objects throws an
        /// exception any locks acquired by the function will be released before
        /// the function exits.
        ///
        /// @throws exception Any exceptions thrown by calling lock() or
        /// try_lock() on the supplied Lockable objects
        ///
        /// @post All the supplied Lockable objects are locked by the calling
        /// thread.
        ///
        /// @see
        /// https://www.boost.org/doc/libs/1_78_0/doc/html/thread/synchronization.html#thread.synchronization.lock_functions
        ///
        /// @tparam Iterator Range iterator type
        /// @param first Iterator to first mutex in the range
        /// @param last Iterator to one past the last mutex in the range
        template <
            class Iterator,
            std::enable_if_t<is_input_iterator_v<Iterator>, int> = 0>
        void
        lock(Iterator first, Iterator last) {
            using lock_type = typename std::iterator_traits<
                Iterator>::value_type;

            /// Auxiliary lock guard for a range of mutexes recursively using
            /// this lock function
            struct range_lock_guard {
                /// Iterator to first locked mutex in the range
                Iterator begin;

                /// Iterator to one past last locked mutex in the range
                Iterator end;

                /// Construct a lock guard for a range of mutexes
                range_lock_guard(Iterator first, Iterator last)
                    : begin(first)
                    , end(last) {
                    // The range lock guard recursively calls the same lock
                    // function we use here
                    futures::detail::lock(begin, end);
                }

                range_lock_guard(range_lock_guard const &) = delete;
                range_lock_guard &
                operator=(range_lock_guard const &)
                    = delete;

                range_lock_guard(range_lock_guard &&other) noexcept
                    : begin(std::exchange(other.begin, Iterator{}))
                    , end(std::exchange(other.end, Iterator{})) {}

                range_lock_guard &
                operator=(range_lock_guard &&other) noexcept {
                    if (this == &other) {
                        return *this;
                    }
                    begin = std::exchange(other.begin, Iterator{});
                    end = std::exchange(other.end, Iterator{});
                    return *this;
                };

                /// Unlock each mutex in the range
                ~range_lock_guard() {
                    while (begin != end) {
                        begin->unlock();
                        ++begin;
                    }
                }

                /// Make the range empty so nothing is unlocked at destruction
                void
                release() {
                    begin = end;
                }
            };

            // Handle trivial cases
            if (first == last) {
                // empty_range
                return;
            } else if (std::next(first) == last) {
                // single_element
                first->lock();
                return;
            }

            // Handle general case
            // One of the locking strategies we are trying
            bool currently_using_first_strategy = true;

            // Define the two ranges first < second <= next < last
            Iterator second = std::next(first);
            Iterator next = second;

            // Alternate between two locking strategies
            for (;;) {
                // A deferred lock assumes the algorithm might lock the first
                // lock later
                std::unique_lock<lock_type> first_lock(*first, std::defer_lock);
                if (currently_using_first_strategy) {
                    // First strategy: Lock first, then _try_ to lock the others
                    first_lock.lock();
                    const Iterator failed_lock_it = try_lock(next, last);
                    if (failed_lock_it == last) {
                        // no_lock_failed: !SUCCESS!
                        // Breaks the association of the associated mutex (i.e.
                        // don't unlock first_lock)
                        first_lock.release();
                        return;
                    } else {
                        // !FAIL!
                        // Try another strategy in the next iteration
                        currently_using_first_strategy = false;
                        next = failed_lock_it;
                    }
                } else {
                    // Second strategy: Lock others, then try to lock first
                    // Create range lock guard for the range [next, last)
                    range_lock_guard range_guard(next, last);
                    // If we can lock first
                    if (first_lock.try_lock()) {
                        // Try to lock [second, next)
                        const Iterator failed_lock = try_lock(second, next);
                        if (failed_lock == next) {
                            // all_locked: !SUCCESS!
                            // Don't let it unlock
                            first_lock.release();
                            range_guard.release();
                            return;
                        } else {
                            // Try this strategy again with a new "next"
                            currently_using_first_strategy = false;
                            next = failed_lock;
                        }
                    } else {
                        // We couldn't lock the first mutex, move to first
                        // strategy
                        currently_using_first_strategy = true;
                        next = second;
                    }
                }
            }
        }

        /** @} */ // @addtogroup futures Futures
    }             // namespace detail
} // namespace futures

#endif // FUTURES_DETAIL_THREAD_LOCK_HPP
