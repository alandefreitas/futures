//
// Copyright (c) 2021 alandefreitas (alandefreitas@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//

#ifndef FUTURES_WAITER_FOR_ANY_H
#define FUTURES_WAITER_FOR_ANY_H

#include <futures/futures/detail/lock.hpp>
#include <futures/futures/detail/shared_state.hpp>
#include <utility>

namespace futures::detail {
    /** \addtogroup futures Futures
     *  @{
     */

    /// \brief Helper class to set signals and wait for any future in a sequence
    /// of futures to become ready
    class waiter_for_any
    {
    public:
        /// \brief Construct a waiter_for_any watching zero futures
        waiter_for_any() = default;

        /// \brief Destruct the waiter
        ///
        /// If the waiter is destroyed before we wait for a result, we disable
        /// the future notifications
        ///
        ~waiter_for_any() {
            for (auto const &waiter: waiters_) {
                waiter.disable_notification();
            }
        }

        /// \brief Construct a waiter_for_any that waits for one of the futures
        /// in a range of futures
        template <typename Iterator>
        waiter_for_any(Iterator first, Iterator last) {
            for (Iterator current = first; current != last; ++current) {
                add(*current);
            }
        }

        waiter_for_any(const waiter_for_any &) = delete;
        waiter_for_any(waiter_for_any &&) = delete;
        waiter_for_any &
        operator=(const waiter_for_any &)
            = delete;
        waiter_for_any &
        operator=(waiter_for_any &&)
            = delete;

        /// \brief Watch the specified future
        template <typename F>
        void
        add(F &f) {
            if constexpr (has_ready_notifier_v<std::decay_t<F>>) {
                if (f.valid()) {
                    registered_waiter
                        waiter(f, f.notify_when_ready(cv), future_count);
                    try {
                        waiters_.push_back(waiter);
                    }
                    catch (...) {
                        f.unnotify_when_ready(waiter.handle);
                        throw;
                    }
                    ++future_count;
                }
            } else {
                // The future has no ready-notifier, so we create a future to
                // poll until it can notify us This is the future we wait for
                // instead
                poller_futures_.emplace_back(futures::async([f = &f]() {
                    f->wait();
                }));
                add(poller_futures_.back());
            }
        }

        /// \brief Watch the specified futures in the parameter pack
        template <typename F1, typename... Fs>
        void
        add(F1 &&f1, Fs &&...fs) {
            add(std::forward<F1>(f1));
            add(std::forward<Fs>(fs)...);
        }

        /// \brief Wait for one of the futures to notify it got ready
        std::size_t
        wait() {
            registered_waiter_range_lock lk(waiters_);
            std::size_t ready_idx;
            cv.wait(lk, [this, &ready_idx]() {
                for (auto const &waiter: waiters_) {
                    if (waiter.is_ready()) {
                        ready_idx = waiter.index;
                        return true;
                    }
                }
                return false;
            });
            return ready_idx;
        }

    private:
        /// \brief Type of handle in the future object used to notify completion
        using notify_when_ready_handle = typename detail::shared_state_base::
            notify_when_ready_handle;

        /// \brief Helper class to store information about each of the futures
        /// we are waiting for
        ///
        /// Because the waiter can be associated with futures of different
        /// types, this class also nullifies the operations necessary to check
        /// the state of the future object.
        ///
        struct registered_waiter
        {
            /// \brief Mutex associated with a future we are watching
            std::mutex *future_mutex_;

            /// \brief Callback to disable notifications
            std::function<void(notify_when_ready_handle)>
                disable_notification_callback;

            /// \brief Callback to disable notifications
            std::function<bool()> is_ready_callback;

            /// \brief Handler to the resource that will notify us when the
            /// future is ready
            ///
            /// In the shared state, this usually represents a pointer to the
            /// condition variable
            ///
            notify_when_ready_handle handle;

            /// \brief Index to this future in the underlying range
            std::size_t index;

            /// \brief Construct a registered waiter to be enqueued in the main
            /// waiter
            template <class Future>
            registered_waiter(
                Future &a_future,
                const notify_when_ready_handle &handle_,
                std::size_t index_)
                : future_mutex_(&a_future.mutex()),
                  disable_notification_callback(
                      [future_ = &a_future](notify_when_ready_handle h)
                          -> void { future_->unnotify_when_ready(h); }),
                  is_ready_callback([future_ = &a_future]() -> bool {
                      return future_->is_ready();
                  }),
                  handle(handle_), index(index_) {}

            /// \brief Get the mutex associated with the future we are watching
            [[nodiscard]] std::mutex &
            mutex() const {
                return *future_mutex_;
            }

            /// \brief Disable notification when the future is ready
            void
            disable_notification() const {
                disable_notification_callback(handle);
            }

            /// \brief Check if underlying future is ready
            [[nodiscard]] bool
            is_ready() const {
                return is_ready_callback();
            }
        };

        /// \brief Helper class to lock all futures
        struct registered_waiter_range_lock
        {
            /// \brief Type for a vector of locks
            using lock_vector = std::vector<std::unique_lock<std::mutex>>;

            /// \brief Type for a shared vector of locks
            using shared_lock_vector = std::shared_ptr<lock_vector>;

            /// \brief Number of futures locked
            std::size_t count{ 0 };

            /// \brief Locks for each future in the range
            shared_lock_vector locks;

            /// \brief Create a lock for each future in the specified vector of
            /// registered waiters
            template <typename WaiterIterator>
            explicit registered_waiter_range_lock(
                WaiterIterator first_waiter,
                WaiterIterator last_waiter)
                : count(std::distance(first_waiter, last_waiter)),
                  locks(std::make_shared<lock_vector>(count)) {
                WaiterIterator waiter_it = first_waiter;
                std::size_t lock_idx = 0;
                while (waiter_it != last_waiter) {
                    (*locks)[lock_idx] = (std::unique_lock<std::mutex>(
                        waiter_it->mutex()));
                    ++waiter_it;
                    ++lock_idx;
                }
            }

            template <typename Range>
            explicit registered_waiter_range_lock(Range &&r)
                : registered_waiter_range_lock(
                    std::forward<Range>(r).begin(),
                    std::forward<Range>(r).end()) {}


            /// \brief Lock all future mutexes in the range
            void
            lock() const {
                futures::detail::lock(locks->begin(), locks->end());
            }

            /// \brief Unlock all future mutexes in the range
            void
            unlock() const {
                for (size_t i = 0; i < count; ++i) {
                    (*locks)[i].unlock();
                }
            }
        };

        /// \brief Condition variable to warn about any ready future
        std::condition_variable_any cv;

        /// \brief Waiters with information about each future and notification
        /// handlers
        std::vector<registered_waiter> waiters_;

        /// \brief Number of futures in this range
        std::size_t future_count{ 0 };

        /// \brief Poller futures
        ///
        /// Futures that support notifications wrapping future types that don't
        ///
        small_vector<cfuture<void>> poller_futures_{};
    };

    /** @} */
} // namespace futures::detail

#endif // FUTURES_WAITER_FOR_ANY_H
