//
// Copyright (c) alandefreitas 11/30/21.
// See accompanying file LICENSE
//

#ifndef FUTURES_SHARED_STATE_H
#define FUTURES_SHARED_STATE_H

#include <atomic>
#include <condition_variable>
#include <future>

#include <futures/futures/detail/intrusive_ptr.h>
#include <futures/futures/future_error.h>

namespace futures::detail {
    /** \addtogroup futures Futures
     *  @{
     */

    /// \brief Abstract class with the members common to all shared state objects regardless of type
    ///
    /// All objects such as futures and promises are shared states and inherit from this class to synchronize
    /// their access to their common shared state.
    ///
    /// All shared states need a counter identifying how many objects are relying
    /// on this shared state and synchronization primitives to set the state.
    class shared_state_base {
      public:
        /// \brief A default constructed shared state data
        shared_state_base() = default;

        /// \brief Cannot copy construct the shared state data
        /// We cannot copy construct the shared state data because this parent class
        /// holds synchronization primitives
        shared_state_base(shared_state_base const &) = delete;

        /// \brief Virtual shared state data destructor
        /// Virtual to make it inheritable
        virtual ~shared_state_base() = default;

        /// \brief Cannot copy assign the shared state data
        /// We cannot copy assign the shared state data because this parent class
        /// holds synchronization primitives
        shared_state_base &operator=(shared_state_base const &) = delete;

        /// \brief Indicate to the shared state its owner has been destroyed
        /// This overload uses the default global mutex for synchronization
        void signal_owner_destroyed() {
            std::unique_lock lk{mtx_};
            signal_owner_destroyed(lk);
        }

        /// \brief Set shared state to an exception
        /// This overload uses the default global mutex for synchronization
        void set_exception(std::exception_ptr except) { // NOLINT(performance-unnecessary-value-param)
            std::unique_lock lk{mtx_};
            set_exception(except, lk);
        }

        /// \brief Get shared state when it's an exception
        /// This overload uses the default global mutex for synchronization
        std::exception_ptr get_exception_ptr() {
            std::unique_lock lk{mtx_};
            return get_exception_ptr(lk);
        }

        /// \brief Check if state is ready
        /// This overload uses the default global mutex for synchronization
        bool is_ready() const {
            std::unique_lock lk{mtx_};
            return is_ready(lk);
        }

        /// \brief Wait for shared state to become ready
        /// This overload uses the default global mutex for synchronization
        void wait() const {
            std::unique_lock lk{mtx_};
            wait(lk);
        }

        /// \brief Wait for a specified duration for the shared state to become ready
        /// This overload uses the default global mutex for synchronization
        template <typename Rep, typename Period>
        std::future_status wait_for(std::chrono::duration<Rep, Period> const &timeout_duration) const {
            std::unique_lock lk{mtx_};
            return wait_for(lk, timeout_duration);
        }

        /// \brief Wait until a specified time point for the shared state to become ready
        /// This overload uses the default global mutex for synchronization
        template <typename Clock, typename Duration>
        std::future_status wait_until(std::chrono::time_point<Clock, Duration> const &timeout_time) const {
            std::unique_lock lk{mtx_};
            return wait_until(lk, timeout_time);
        }

        /// \brief Count how many objects refer to the shared state
        std::size_t use_count() noexcept {
            return use_count_.load();
        }

        /// \brief Increment the reference counter for the number of objects sharing this state
        /// This function is used by intrusive pointers to keep track of the number of references
        /// to this shared state. This is only an optimization over shared pointers, whose
        /// reference counter would be redundant with use_count_ and would require extra
        /// synchronization.
        ///
        /// There are no synchronization or ordering constraints imposed on this operation
        friend inline void intrusive_ptr_add_ref(shared_state_base *p) noexcept {
            p->use_count_.fetch_add(1, std::memory_order_relaxed);
        }

        /// \brief Decrement the reference counter for number of objects sharing this state
        /// This function is used by intrusive pointers to keep track of the number of references
        /// to this shared state. This is only an optimization over shared pointers, whose
        /// reference counter would be redundant with use_count_ and would require extra
        /// synchronization.
        ///
        /// If the value immediately preceding this decrement on the number of objects was 1
        /// (i.e. the current number of objects is 0), we deallocate the future associated
        /// with this shared state.
        ///
        /// No reads or writes in the decrement operation can be reordered after this store.
        friend inline void intrusive_ptr_release(shared_state_base *p) noexcept {
            if (1 == p->use_count_.fetch_sub(1, std::memory_order_release)) {
                std::atomic_thread_fence(std::memory_order_acquire);
                p->deallocate_future();
            }
        }

      protected:
        /// \brief Indicate to the shared state the state is ready in the derived class
        /// This operation marks the ready_ flags and warns any future waiting on it.
        /// This overload is meant to be used by derived classes that might need to use another mutex for this operation
        void mark_ready_and_notify(std::unique_lock<std::mutex> &lk) noexcept {
            assert(lk.owns_lock());
            ready_ = true;
            lk.unlock();
            waiters_.notify_all();
        }

        /// \brief Indicate to the shared state its owner has been destroyed
        /// If owner has been destroyed before the shared state is ready, this means a promise has been broken
        /// and the shared state should store an exception.
        /// This overload is meant to be used by derived classes that might need to use another mutex for this operation
        void signal_owner_destroyed(std::unique_lock<std::mutex> &lk) {
            assert(lk.owns_lock());
            if (!ready_) {
                set_exception(std::make_exception_ptr(broken_promise()), lk);
            }
        }

        /// \brief Set shared state to an exception
        /// This sets the exception value and marks the shared state as ready.
        /// If we try to set an exception on a shared state that's ready, we throw an exception.
        /// This overload is meant to be used by derived classes that might need to use another mutex for this operation
        void set_exception(std::exception_ptr except, std::unique_lock<std::mutex> &lk) {
            assert(lk.owns_lock());
            if (ready_) {
                throw promise_already_satisfied();
            }
            except_ = except;

            mark_ready_and_notify(lk);
        }

        /// \brief Get shared state when it's an exception
        /// This overload is meant to be used by derived classes that might need to use another mutex for this operation
        std::exception_ptr get_exception_ptr(std::unique_lock<std::mutex> &lk) {
            assert(lk.owns_lock());
            wait(lk);
            return except_;
        }

        /// \brief Check if state is ready
        bool is_ready([[maybe_unused]] const std::unique_lock<std::mutex> &lk) const {
            assert(lk.owns_lock());
            return ready_;
        }

        /// \brief Wait for shared state to become ready
        /// This function uses the condition variable waiters to wait for this shared state to be marked as ready
        /// This overload is meant to be used by derived classes that might need to use another mutex for this operation
        void wait(std::unique_lock<std::mutex> &lk) const {
            assert(lk.owns_lock());
            waiters_.wait(lk, [this]() { return ready_; });
        }

        /// \brief Wait for a specified duration for the shared state to become ready
        /// This function uses the condition variable waiters to wait for this shared state to be marked as ready
        /// for a specified duration.
        /// This overload is meant to be used by derived classes that might need to use another mutex for this operation
        template <typename Rep, typename Period>
        std::future_status wait_for(std::unique_lock<std::mutex> &lk,
                                    std::chrono::duration<Rep, Period> const &timeout_duration) const {
            assert(lk.owns_lock());
            if (waiters_.wait_for(lk, timeout_duration, [this]() { return ready_; })) {
                return std::future_status::ready;
            } else {
                return std::future_status::timeout;
            }
        }

        /// \brief Wait until a specified time point for the shared state to become ready
        /// This function uses the condition variable waiters to wait for this shared state to be marked as ready
        /// until a specified time point.
        /// This overload is meant to be used by derived classes that might need to use another mutex for this operation
        template <typename Clock, typename Duration>
        std::future_status wait_until(std::unique_lock<std::mutex> &lk,
                                      std::chrono::time_point<Clock, Duration> const &timeout_time) const {
            assert(lk.owns_lock());
            return waiters_.wait_until(lk, timeout_time, [this]() { return ready_; }) ? std::future_status::ready
                                                                                      : std::future_status::timeout;
        }

        /// \brief Virtual function to deallocate future
        /// This is the pure virtual function derived classes can use to deallocate the state associated
        /// with the future.
        virtual void deallocate_future() noexcept = 0;

      protected:
        /// \brief Default global mutex for shared state operations
        mutable std::mutex mtx_{};

        /// \brief Indicates if the shared state is already set
        bool ready_{false};

        /// \brief Pointer to an exception, when the shared_state has been set to an exception
        std::exception_ptr except_{};

      private:
        /// \brief Number of objects referring to this shared state
        std::atomic<std::size_t> use_count_{0};

        /// \brief Condition variable to notify any object waiting for this shared state to be ready
        mutable std::condition_variable waiters_{};
    };

    /// \brief Abstract class with shared state for regular variables
    /// This class stores the data for a shared state holding an element of type T
    template <typename R> class shared_state : public shared_state_base {
      public:
        /// \brief Pointer to the shared state
        /// This intrusive pointer is a fast replacement for a shared pointer
        using ptr_type = intrusive_ptr<shared_state>;

        /// \brief Default construct a shared state
        /// This will construct the state with uninitialized storage for R
        shared_state() = default;

        /// \brief Destructor
        /// Destruct the shared object R if the state is ready with a value
        ~shared_state() override {
            if (ready_ && !except_) {
                reinterpret_cast<R *>(std::addressof(storage_))->~R();
            }
        }

        /// \brief Deleted copy constructor
        /// These functions do not make sense for shared states as they are shared
        shared_state(shared_state const &) = delete;

        /// \brief Deleted copy assignment
        /// These functions do not make sense for shared states as they are shared
        shared_state &operator=(shared_state const &) = delete;

        /// \brief Set the value of the shared state to a copy of value
        /// This function locks the shared state and makes a copy of value
        void set_value(R const &value) {
            std::unique_lock lk{mtx_};
            set_value(value, lk);
        }

        /// \brief Set the value of the shared state by moving a value
        /// This function locks the shared state and moves the value to the state
        void set_value(R &&value) {
            std::unique_lock lk{mtx_};
            set_value(std::move(value), lk);
        }

        /// \brief Get the value of the shared state
        /// \return Reference to the state as a reference to R
        R &get() {
            std::unique_lock lk{mtx_};
            return get(lk);
        }

      private:
        /// \brief Set value of the shared state in the storage by copying the value
        /// \param value The value for the shared state
        /// \param lk Custom mutex lock
        void set_value(R const &value, std::unique_lock<std::mutex> &lk) {
            assert(lk.owns_lock());
            if (ready_) {
                throw promise_already_satisfied{};
            }
            ::new (static_cast<void *>(std::addressof(storage_))) R(value);
            mark_ready_and_notify(lk);
        }

        /// \brief Set value of the shared state in the storage by moving the value
        /// \param value The rvalue for the shared state
        /// \param lk Custom mutex lock
        void set_value(R &&value, std::unique_lock<std::mutex> &lk) {
            assert(lk.owns_lock());
            if (ready_) {
                throw promise_already_satisfied{};
            }
            ::new (static_cast<void *>(std::addressof(storage_))) R(std::move(value));
            mark_ready_and_notify(lk);
        }

        /// \brief Get value of the shared state
        /// This function wait for the shared state to become ready and returns its value
        /// \param lk Custom mutex lock
        /// \return Shared state as a reference to R
        R &get(std::unique_lock<std::mutex> &lk) {
            assert(lk.owns_lock());
            wait(lk);
            if (except_) {
                std::rethrow_exception(except_);
            }
            return *reinterpret_cast<R *>(std::addressof(storage_));
        }

        /// \brief Aligned opaque storage for an element of type R
        /// This needs to be aligned_storage_t because the value might not be initialized yet
        typename std::aligned_storage_t<sizeof(R), alignof(R)> storage_{};
    };

    /// \brief Abstract class with shared state for references
    /// This class stores the data for a shared state holding a reference to an element of type T
    /// These shared states need to store a pointer to R internally
    template <typename R> class shared_state<R &> : public shared_state_base {
      public:
        /// \brief Pointer to the shared state
        /// This intrusive pointer is a fast replacement for a shared pointer
        using ptr_type = intrusive_ptr<shared_state>;

        /// \brief Default construct a shared state
        /// This will construct the state with uninitialized storage for R
        shared_state() = default;

        /// \brief Destructor
        /// Destruct the shared object R if the state is ready with a value
        ~shared_state() override = default;

        /// \brief Deleted copy constructor
        /// These functions do not make sense for shared states as they are shared
        shared_state(shared_state const &) = delete;

        /// \brief Deleted copy assignment
        /// These functions do not make sense for shared states as they are shared
        shared_state &operator=(shared_state const &) = delete;

        /// \brief Set the value of the shared state by moving a value R to the internal state
        /// This function locks the shared state and moves the value to the state
        void set_value(R &value) {
            std::unique_lock lk{mtx_};
            set_value(value, lk);
        }

        /// \brief Get the value of the shared state as a reference to the internal pointer
        /// \return Reference to the state as a reference to R
        R &get() {
            std::unique_lock lk{mtx_};
            return get(lk);
        }

      private:
        /// \brief Set reference of the shared state to the address of a value R
        /// \param value The value for the shared state
        /// \param lk Custom mutex lock
        void set_value(R &value, std::unique_lock<std::mutex> &lk) {
            assert(lk.owns_lock());
            if (ready_) {
                throw promise_already_satisfied();
            }
            value_ = std::addressof(value);
            mark_ready_and_notify(lk);
        }

        /// \brief Get reference to the shared value
        /// \return Reference to the state as a reference to R
        R &get(std::unique_lock<std::mutex> &lk) {
            assert(lk.owns_lock());
            wait(lk);
            if (except_) {
                std::rethrow_exception(except_);
            }
            return *value_;
        }

        /// \brief Pointer to an element of type R representing the reference
        /// This needs to be aligned_storage_t because the value might not be initialized yet
        R *value_{nullptr};
    };

    /// \brief Abstract class with shared state for a void shared state
    /// A void shared state needs to synchronize waiting but it does not need to store anything
    template <> class shared_state<void> : public shared_state_base {
      public:
        /// \brief Pointer to the shared state
        /// This intrusive pointer is a fast replacement for a shared pointer
        using ptr_type = intrusive_ptr<shared_state>;

        /// \brief Default construct a shared state
        /// This will construct the state with uninitialized storage for R
        shared_state() = default;

        /// \brief Destructor
        /// Destruct the shared object R if the state is ready with a value
        ~shared_state() override = default;

        /// \brief Deleted copy constructor
        /// These functions do not make sense for shared states as they are shared
        shared_state(shared_state const &) = delete;

        /// \brief Deleted copy assignment
        /// These functions do not make sense for shared states as they are shared
        shared_state &operator=(shared_state const &) = delete;

        /// \brief Set the value of the void shared state without any input as "not-error"
        /// This function locks the shared state and moves the value to the state
        inline void set_value() {
            std::unique_lock lk{mtx_};
            set_value(lk);
        }

        /// \brief Get the value of the shared state by waiting and checking for exceptions
        /// \return Reference to the state as a reference to R
        inline void get() {
            std::unique_lock lk{mtx_};
            get(lk);
        }

      private:
        /// \brief Set value of the shared state with no inputs as "no-error"
        /// \param lk Custom mutex lock
        inline void set_value(std::unique_lock<std::mutex> &lk) {
            assert(lk.owns_lock());
            if (ready_) {
                throw promise_already_satisfied();
            }
            mark_ready_and_notify(lk);
        }

        /// \brief Get the value of the shared state by waiting and checking for exceptions
        void get(std::unique_lock<std::mutex> &lk) const {
            assert(lk.owns_lock());
            wait(lk);
            if (except_) {
                std::rethrow_exception(except_);
            }
        }
    };

    /** @} */ // \addtogroup futures Futures
} // namespace futures::detail

#endif // FUTURES_SHARED_STATE_H
