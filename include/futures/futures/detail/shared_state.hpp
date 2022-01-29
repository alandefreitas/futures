//
// Copyright (c) 2021 alandefreitas (alandefreitas@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//

#ifndef FUTURES_SHARED_STATE_H
#define FUTURES_SHARED_STATE_H

#include <futures/futures/future_error.hpp>
#include <futures/futures/detail/relocker.hpp>
#include <futures/futures/detail/small_vector.hpp>
#include <atomic>
#include <future>
#include <condition_variable>

namespace futures {
    /** \addtogroup futures Futures
     *  @{
     */

    namespace detail {
        /// \brief Members functions and objects common to all shared state
        /// object types (bool ready and exception ptr)
        ///
        /// Shared states for asynchronous operations contain an element of a
        /// given type and an exception.
        ///
        /// All objects such as futures and promises have shared states and
        /// inherit from this class to synchronize their access to their common
        /// shared state.
        class shared_state_base
        {
        public:
            /// \brief A list of waiters: condition variables to notify any
            /// object waiting for this shared state to be ready
            using waiter_list = detail::small_vector<
                std::condition_variable_any *>;

            /// \brief A handle to notify an external context about this state
            /// being ready
            using notify_when_ready_handle = waiter_list::iterator;

            /// \brief A default constructed shared state data
            shared_state_base() = default;

            /// \brief Cannot copy construct the shared state data
            ///
            /// We cannot copy construct the shared state data because its
            /// parent class holds synchronization primitives
            shared_state_base(const shared_state_base &) = delete;

            /// \brief Virtual shared state data destructor
            ///
            /// Virtual to make it inheritable
            virtual ~shared_state_base() = default;

            /// \brief Cannot copy assign the shared state data
            ///
            /// We cannot copy assign the shared state data because this parent
            /// class holds synchronization primitives
            shared_state_base &
            operator=(const shared_state_base &)
                = delete;

            /// \brief Indicate to the shared state the state is ready in the
            /// derived class
            ///
            /// This operation marks the ready_ flags and warns any future
            /// waiting on it. This overload is meant to be used by derived
            /// classes that might need to use another mutex for this operation
            void
            set_ready() noexcept {
                status prev = status_.exchange(
                    status::ready,
                    std::memory_order_release);
                if (prev == status::waiting) {
                    std::atomic_thread_fence(std::memory_order_acquire);
                    auto lk = create_wait_lock();
                    waiter_.notify_all();
                    for (auto &&external_waiter: external_waiters_) {
                        external_waiter->notify_all();
                    }
                }
            }

            /// \brief Check if state is ready
            ///
            /// This overload uses the default global mutex for synchronization
            bool
            is_ready() const {
                return status_.load(std::memory_order_acquire) == status::ready;
            }

            /// \brief Check if state is ready without locking
            ///
            /// Although the parent shared state class doesn't store the state,
            /// we know it's ready with state because it's the only way it's
            /// ready unless it has an exception.
            bool
            succeeded() const {
                return is_ready() && !except_;
            }

            /// \brief Set shared state to an exception
            ///
            /// This sets the exception value and marks the shared state as
            /// ready. If we try to set an exception on a shared state that's
            /// ready, we throw an exception. This overload is meant to be used
            /// by derived classes that might need to use another mutex for this
            /// operation.
            ///
            void
            set_exception(std::exception_ptr except) {
                if (status_.load(std::memory_order_acquire) == status::ready) {
                    detail::throw_exception<promise_already_satisfied>();
                }
                // ready_ already protects except_ because only one thread
                // should call this function
                except_ = std::move(except);
                set_ready();
            }

            /// \brief Get the shared state when it's an exception
            ///
            /// This overload uses the default global mutex for synchronization
            std::exception_ptr
            get_exception_ptr() const {
                if (status_.load(std::memory_order_acquire) != status::ready) {
                    detail::throw_exception<promise_uninitialized>();
                }
                return except_;
            }

            /// \brief Rethrow the exception we have stored
            void
            throw_internal_exception() const {
                std::rethrow_exception(get_exception_ptr());
            }

            /// \brief Indicate to the shared state its owner has been destroyed
            ///
            /// If owner has been destroyed before the shared state is ready,
            /// this means a promise has been broken and the shared state should
            /// store an exception. This overload is meant to be used by derived
            /// classes that might need to use another mutex for this operation
            void
            signal_promise_destroyed() {
                if (status_.load(std::memory_order_acquire) != status::ready) {
                    set_exception(std::make_exception_ptr(broken_promise()));
                }
            }

            /// \brief Check if state is ready without locking
            bool
            failed() const {
                return is_ready() && except_ != nullptr;
            }

            /// \brief Wait for shared state to become ready
            ///
            /// This overload uses the default global mutex for synchronization
            void
            wait() const {
                status prev = status::initial;
                status_.compare_exchange_strong(prev, status::waiting);
                if (prev != status::ready) {
                    auto lk = create_wait_lock();
                    waiter_.wait(lk, [this]() {
                        return (
                            status_.load(std::memory_order_acquire)
                            == status::ready);
                    });
                }
            }

            /// \brief Wait for shared state to become ready
            ///
            /// This function uses the condition variable waiters to wait for
            /// this shared state to be marked as ready This overload is meant
            /// to be used by derived classes that might need to use another
            /// mutex for this operation
            ///
            void
            wait(std::unique_lock<std::mutex> &lk) const {
                status prev = status::initial;
                status_.compare_exchange_strong(prev, status::waiting);
                if (prev != status::ready) {
                    assert(lk.owns_lock());
                    waiter_.wait(lk, [this]() {
                        return (
                            status_.load(std::memory_order_acquire)
                            == status::ready);
                    });
                }
            }

            /// \brief Wait for a specified duration for the shared state to
            /// become ready
            ///
            /// This overload uses the default global mutex for synchronization
            template <typename Rep, typename Period>
            std::future_status
            wait_for(std::chrono::duration<Rep, Period> const &timeout_duration)
                const {
                status prev = status::initial;
                status_.compare_exchange_strong(prev, status::waiting);
                if (prev != status::ready) {
                    auto lk = create_wait_lock();
                    if (waiter_.wait_for(lk, timeout_duration, [this]() {
                            return status_.load(std::memory_order_acquire)
                                   == status::ready;
                        }))
                    {
                        return std::future_status::ready;
                    } else {
                        return std::future_status::timeout;
                    }
                }
                return std::future_status::ready;
            }

            /// \brief Wait for a specified duration for the shared state to
            /// become ready
            ///
            /// This function uses the condition variable waiters to wait for
            /// this shared state to be marked as ready for a specified
            /// duration. This overload is meant to be used by derived classes
            /// that might need to use another mutex for this operation
            template <typename Rep, typename Period>
            std::future_status
            wait_for(
                std::unique_lock<std::mutex> &lk,
                std::chrono::duration<Rep, Period> const &timeout_duration)
                const {
                status prev = status::initial;
                status_.compare_exchange_strong(prev, status::waiting);
                if (prev != status::ready) {
                    assert(lk.owns_lock());
                    if (waiter_.wait_for(lk, timeout_duration, [this]() {
                            return status_.load(std::memory_order_acquire)
                                   == status::ready;
                        }))
                    {
                        return std::future_status::ready;
                    } else {
                        return std::future_status::timeout;
                    }
                }
                return std::future_status::ready;
            }

            /// \brief Wait until a specified time point for the shared state to
            /// become ready
            ///
            /// This overload uses the default global mutex for synchronization
            template <typename Clock, typename Duration>
            std::future_status
            wait_until(std::chrono::time_point<Clock, Duration> const
                           &timeout_time) const {
                status prev = status::initial;
                status_.compare_exchange_strong(prev, status::waiting);
                if (prev != status::ready) {
                    auto lk = create_wait_lock();
                    if (waiter_.wait_until(lk, timeout_time, [this]() {
                            return status_.load(std::memory_order_acquire)
                                   == status::ready;
                        }))
                    {
                        return std::future_status::ready;
                    } else {
                        return std::future_status::timeout;
                    }
                }
                return std::future_status::ready;
            }

            /// \brief Wait until a specified time point for the shared state to
            /// become ready This function uses the condition variable waiters
            /// to wait for this shared state to be marked as ready until a
            /// specified time point. This overload is meant to be used by
            /// derived classes that might need to use another mutex for this
            /// operation
            template <typename Clock, typename Duration>
            std::future_status
            wait_until(
                std::unique_lock<std::mutex> &lk,
                std::chrono::time_point<Clock, Duration> const &timeout_time)
                const {
                status prev = status::initial;
                status_.compare_exchange_strong(prev, status::waiting);
                if (prev != status::ready) {
                    assert(lk.owns_lock());
                    if (waiter_.wait_until(lk, timeout_time, [this]() {
                            return status_.load(std::memory_order_acquire)
                                   == status::ready;
                        }))
                    {
                        return std::future_status::ready;
                    } else {
                        return std::future_status::timeout;
                    }
                }
                return std::future_status::ready;
            }

            /// \brief Call the internal callback function, if any
            void
            do_callback(std::unique_lock<std::mutex> &lk) const {
                if (callback_
                    && !(
                        status_.load(std::memory_order_acquire)
                        == status::ready)) {
                    std::function<void()> local_callback = callback_;
                    relocker relock(lk);
                    local_callback();
                }
            }

            /// \brief Include a condition variable in the list of waiters we
            /// need to notify when the state is ready
            notify_when_ready_handle
            notify_when_ready(std::condition_variable_any &cv) {
                status expected = status::initial;
                status_.compare_exchange_strong(expected, status::waiting);
                auto lk = create_wait_lock();
                do_callback(lk);
                return external_waiters_.insert(external_waiters_.end(), &cv);
            }

            /// \brief Include a condition variable in the list of waiters we
            /// need to notify when the state is ready
            notify_when_ready_handle
            notify_when_ready(
                [[maybe_unused]] std::unique_lock<std::mutex> &lk,
                std::condition_variable_any &cv) {
                status expected = status::initial;
                status_.compare_exchange_strong(expected, status::waiting);
                assert(lk.owns_lock());
                do_callback(lk);
                return external_waiters_.insert(external_waiters_.end(), &cv);
            }

            /// \brief Remove condition variable from list of condition
            /// variables we need to warn about this state
            void
            unnotify_when_ready(notify_when_ready_handle it) {
                status expected = status::initial;
                status_.compare_exchange_strong(expected, status::waiting);
                auto lk = create_wait_lock();
                external_waiters_.erase(it);
            }

            /// \brief Remove condition variable from list of condition
            /// variables we need to warn about this state
            void
            unnotify_when_ready(
                [[maybe_unused]] std::unique_lock<std::mutex> &lk,
                notify_when_ready_handle it) {
                status expected = status::initial;
                status_.compare_exchange_strong(expected, status::waiting);
                assert(lk.owns_lock());
                external_waiters_.erase(it);
            }

            /// \brief Get a reference to the mutex in the shared state
            std::mutex &
            mutex() {
                return waiters_mutex_;
            }

            /// \brief Generate unique lock for the shared state
            ///
            /// This lock can be used for any operations on the state that might
            /// need to be protected/
            std::unique_lock<std::mutex>
            create_wait_lock() const {
                return std::unique_lock{ waiters_mutex_ };
            }

        private:
            /// \brief The current status of this shared state
            enum status : uint8_t
            {
                initial,
                waiting,
                ready,
            };

            /// \brief Indicates if the shared state is already set
            ///
            /// There are three states possible: nothing, waiting, ready
            mutable std::atomic<status> status_{ status::initial };

            /// \brief Pointer to an exception, when the shared_state fails
            ///
            /// std::exception_ptr does not need to be atomic because
            /// the status variable is guarding it.
            ///
            std::exception_ptr except_{ nullptr };

            /// \brief Condition variable to notify any object waiting for this
            /// shared state to be ready
            ///
            /// This is the object we use to be able to block until the shared
            /// state is ready. Although the shared state is lock-free, users
            /// might still need to wait for results to be ready. One future
            /// thread calls `waiter_.wait(...)` to block while the promise
            /// thread calls `waiter_.notify_all(...)`. In C++20, atomic
            /// variables have `wait()` functions we could use to replace this
            /// with `ready_.wait(...)`
            ///
            mutable std::condition_variable waiter_{};

            /// \brief List of external condition variables also waiting for
            /// this shared state to be ready
            ///
            /// While the internal waiter is intended for
            ///
            waiter_list external_waiters_;

            /// \brief Callback function we should call when waiting for the
            /// shared state
            std::function<void()> callback_;

            /// \brief Mutex for threads that want to wait on the result
            ///
            /// While the shared state is lock-free, it also includes a mutex
            /// that can be used for communication between futures, such as
            /// waiter futures.
            ///
            /// This is used when one thread intents to wait for the result
            /// of a future. The mutex is not used for lazy futures or if
            /// the result is ready before the waiting operation.
            ///
            /// These functions should be used directly by users very often.
            ///
            mutable std::mutex waiters_mutex_{};
        };

        /// \brief Determine the type we should use to store a shared state
        /// internally
        ///
        /// We usually need uninitialized storage for a given type, since the
        /// shared state needs to be in control of constructors and destructors.
        ///
        /// For trivial types, we can directly store the value.
        ///
        /// When the shared state is a reference, we store pointers internally.
        ///
        template <typename R, class Enable = void>
        struct shared_state_storage
        {
            using type = std::aligned_storage_t<sizeof(R), alignof(R)>;
        };

        template <typename R>
        struct shared_state_storage<R, std::enable_if_t<std::is_trivial_v<R>>>
        {
            using type = R;
        };

        template <typename R>
        using shared_state_storage_t = typename shared_state_storage<R>::type;
    } // namespace detail

    /// \brief Shared state specialization for regular variables
    ///
    /// This class stores the data for a shared state holding an element of type
    /// T
    ///
    /// The data is stored with uninitialized storage because this will only
    /// need to be initialized when the state becomes ready. This ensures the
    /// shared state works for all types and avoids wasting operations on a
    /// constructor we might not use.
    ///
    /// However, initialized storage might be more useful and easier to debug
    /// for trivial types.
    ///
    template <typename R>
    class shared_state : public detail::shared_state_base
    {
    public:
        /// \brief Default construct a shared state
        ///
        /// This will construct the state with uninitialized storage for R
        ///
        shared_state() = default;

        /// \brief Deleted copy constructor
        ///
        /// The copy constructor does not make sense for shared states as they
        /// should be shared. Besides, copying state that might be uninitialized
        /// is a bad idea.
        ///
        shared_state(shared_state const &) = delete;

        /// \brief Deleted copy assignment operator
        ///
        /// These functions do not make sense for shared states as they are
        /// shared
        shared_state &
        operator=(shared_state const &)
            = delete;

        /// \brief Destructor
        ///
        /// Destruct the shared object R manually if the state is ready with a
        /// value
        ///
        ~shared_state() override {
            if constexpr (!std::is_trivial_v<R>) {
                if (succeeded()) {
                    reinterpret_cast<R *>(std::addressof(storage_))->~R();
                }
            }
        }

        /// \brief Set the value of the shared state to a copy of value
        ///
        /// This function locks the shared state and makes a copy of the value
        /// into the storage.
        void
        set_value(const R &value) {
            auto lk = create_wait_lock();
            set_value(value, lk);
        }

        /// \brief Set the value of the shared state by moving a value
        ///
        /// This function locks the shared state and moves the value to the state
        void
        set_value(R &&value) {
            auto lk = create_wait_lock();
            set_value(std::move(value), lk);
        }

        /// \brief Get the value of the shared state
        ///
        /// \return Reference to the state as a reference to R
        R &
        get() {
            auto lk = create_wait_lock();
            return get(lk);
        }

    private:
        /// \brief Set value of the shared state in the storage by copying the
        /// value
        ///
        /// \param value The value for the shared state
        /// \param lk Custom mutex lock
        void
        set_value(R const &value, std::unique_lock<std::mutex> &lk) {
            assert(lk.owns_lock());
            if (is_ready()) {
                detail::throw_exception<promise_already_satisfied>();
            }
            ::new (static_cast<void *>(std::addressof(storage_))) R(value);
            detail::relocker relk(lk);
            set_ready();
        }

        /// \brief Set value of the shared state in the storage by moving the
        /// value
        ///
        /// \param value The rvalue for the shared state
        /// \param lk Custom mutex lock
        void
        set_value(R &&value, std::unique_lock<std::mutex> &lk) {
            assert(lk.owns_lock());
            if (is_ready()) {
                detail::throw_exception<promise_already_satisfied>();
            }
            ::new (static_cast<void *>(std::addressof(storage_)))
                R(std::move(value));
            detail::relocker relk(lk);
            set_ready();
        }

        /// \brief Get value of the shared state
        /// This function wait for the shared state to become ready and returns
        /// its value \param lk Custom mutex lock \return Shared state as a
        /// reference to R
        R &
        get(std::unique_lock<std::mutex> &lk) {
            assert(lk.owns_lock());
            wait(lk);
            if (failed()) {
                throw_internal_exception();
            }
            return *reinterpret_cast<R *>(std::addressof(storage_));
        }

        /// \brief Aligned opaque storage for an element of type R
        ///
        /// This needs to be aligned_storage_t because the value might not be
        /// initialized yet
        detail::shared_state_storage_t<R> storage_{};
    };

    /// \brief Shared state specialization for references
    ///
    /// This class stores the data for a shared state holding a reference to an
    /// element of type T These shared states need to store a pointer to R
    /// internally
    template <typename R>
    class shared_state<R &> : public detail::shared_state_base
    {
    public:
        /// \brief Default construct a shared state
        ///
        /// This will construct the state with uninitialized storage for R
        shared_state() = default;

        /// \brief Deleted copy constructor
        ///
        /// These functions do not make sense for shared states as they are
        /// shared
        shared_state(shared_state const &) = delete;

        /// \brief Destructor
        ///
        /// Destruct the shared object R if the state is ready with a value
        ~shared_state() override = default;

        /// \brief Deleted copy assignment
        ///
        /// These functions do not make sense for shared states as they are
        /// shared
        shared_state &
        operator=(shared_state const &)
            = delete;

        /// \brief Set the value of the shared state by storing the address of
        /// value R in its internal state
        ///
        /// Shared states to references internally store a pointer to the value
        /// instead fo copying it. This function locks the shared state and
        /// moves the value to the state
        void
        set_value(std::enable_if_t<!std::is_same_v<R, void>, R> &value) {
            auto lk = create_wait_lock();
            set_value(value, lk);
        }

        /// \brief Get the value of the shared state as a reference to the
        /// internal pointer \return Reference to the state as a reference to R
        R &
        get() {
            auto lk = create_wait_lock();
            return get(lk);
        }

    private:
        /// \brief Set reference of the shared state to the address of a value R
        /// \param value The value for the shared state
        /// \param lk Custom mutex lock
        void
        set_value(R &value, std::unique_lock<std::mutex> &lk) {
            assert(lk.owns_lock());
            if (is_ready()) {
                detail::throw_exception<promise_already_satisfied>();
            }
            value_ = std::addressof(value);
            detail::relocker relk(lk);
            set_ready();
        }

        /// \brief Get reference to the shared value
        /// \return Reference to the state as a reference to R
        R &
        get(std::unique_lock<std::mutex> &lk) {
            assert(lk.owns_lock());
            wait(lk);
            if (failed()) {
                throw_internal_exception();
            }
            return *value_;
        }

        /// \brief Pointer to an element of type R representing the reference
        /// This needs to be aligned_storage_t because the value might not be
        /// initialized yet
        R *value_{ nullptr };
    };

    /// \brief Shared state specialization for a void shared state
    ///
    /// A void shared state needs to synchronize waiting, but it does not need
    /// to store anything.
    template <>
    class shared_state<void> : public detail::shared_state_base
    {
    public:
        /// \brief Default construct a shared state
        ///
        /// This will construct the state with uninitialized storage for R
        shared_state() = default;

        /// \brief Deleted copy constructor
        ///
        /// These functions do not make sense for shared states as they are
        /// shared
        shared_state(shared_state const &) = delete;

        /// \brief Destructor
        ///
        /// Destruct the shared object R if the state is ready with a value
        ~shared_state() override = default;

        /// \brief Deleted copy assignment
        ///
        /// These functions do not make sense for shared states as they are
        /// shared
        shared_state &
        operator=(shared_state const &)
            = delete;

        /// \brief Set the value of the void shared state without any input as
        /// "not-error"
        ///
        /// This function locks the shared state and moves the value to the state
        void
        set_value() {
            auto lk = create_wait_lock();
            set_value(lk);
        }

        /// \brief Get the value of the shared state by waiting and checking for
        /// exceptions
        ///
        /// \return Reference to the state as a reference to R
        void
        get() {
            auto lk = create_wait_lock();
            get(lk);
        }

    private:
        /// \brief Set value of the shared state with no inputs as "no-error"
        ///
        /// \param lk Custom mutex lock
        void
        set_value(std::unique_lock<std::mutex> &lk) {
            assert(lk.owns_lock());
            if (is_ready()) {
                detail::throw_exception<promise_already_satisfied>();
            }
            detail::relocker relk(lk);
            set_ready();
        }

        /// \brief Get the value of the shared state by waiting and checking for
        /// exceptions
        void
        get(std::unique_lock<std::mutex> &lk) const {
            assert(lk.owns_lock());
            wait(lk);
            if (failed()) {
                throw_internal_exception();
            }
        }
    };

    /** @} */ // \addtogroup futures Futures
} // namespace futures

#endif // FUTURES_SHARED_STATE_H
