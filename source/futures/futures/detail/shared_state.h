//
// Copyright (c) alandefreitas 11/30/21.
// See accompanying file LICENSE
//

#ifndef FUTURES_SHARED_STATE_H
#define FUTURES_SHARED_STATE_H

#include <atomic>
#include <condition_variable>
#include <future>

#include <futures/futures/future_error.h>

namespace futures {
    /** \addtogroup futures Futures
     *  @{
     */

    namespace detail {

        /// \brief Members functions and objects common to all shared state object types
        ///
        /// Shared states for asynchronous operations contain an element of a given type and an exception.
        ///
        /// All objects such as futures and promises have shared states and inherit from this class to synchronize
        /// their access to their common shared state.
        class shared_state_base {
          public:
            /// \brief A default constructed shared state data
            shared_state_base() = default;

            /// \brief Cannot copy construct the shared state data
            ///
            /// We cannot copy construct the shared state data because its parent class
            /// holds synchronization primitives
            shared_state_base(const shared_state_base &) = delete;

            /// \brief Virtual shared state data destructor
            ///
            /// Virtual to make it inheritable
            virtual ~shared_state_base() = default;

            /// \brief Cannot copy assign the shared state data
            ///
            /// We cannot copy assign the shared state data because this parent class
            /// holds synchronization primitives
            shared_state_base &operator=(const shared_state_base &) = delete;

            /// \brief Indicate to the shared state its owner (promise or packaged task) has been destroyed
            ///
            /// If the owner of a shared state is destroyed before the shared state is ready, we need to set
            /// an exception indicating this error. This function checks if the shared state is at a correct
            /// state when the owner is destroyed.
            ///
            /// This overload uses the default global mutex for synchronization
            void signal_owner_destroyed() {
                auto lk = create_unique_lock();
                signal_owner_destroyed(lk);
            }

            /// \brief Set shared state to an exception
            ///
            /// This overload uses the default global mutex for synchronization
            void set_exception(std::exception_ptr except) { // NOLINT(performance-unnecessary-value-param)
                auto lk = create_unique_lock();
                set_exception(except, lk);
            }

            /// \brief Get the shared state when it's an exception
            ///
            /// This overload uses the default global mutex for synchronization
            std::exception_ptr get_exception_ptr() {
                auto lk = create_unique_lock();
                return get_exception_ptr(lk);
            }

            /// \brief Check if state is ready
            ///
            /// This overload uses the default global mutex for synchronization
            bool is_ready() const {
                auto lk = create_unique_lock();
                return is_ready(lk);
            }

            /// \brief Wait for shared state to become ready
            ///
            /// This overload uses the default global mutex for synchronization
            void wait() const {
                auto lk = create_unique_lock();
                wait(lk);
            }

            /// \brief Wait for a specified duration for the shared state to become ready
            ///
            /// This overload uses the default global mutex for synchronization
            template <typename Rep, typename Period>
            std::future_status wait_for(std::chrono::duration<Rep, Period> const &timeout_duration) const {
                auto lk = create_unique_lock();
                return wait_for(lk, timeout_duration);
            }

            /// \brief Wait until a specified time point for the shared state to become ready
            ///
            /// This overload uses the default global mutex for synchronization
            template <typename Clock, typename Duration>
            std::future_status wait_until(std::chrono::time_point<Clock, Duration> const &timeout_time) const {
                auto lk = create_unique_lock();
                return wait_until(lk, timeout_time);
            }

          protected:
            /// \brief Indicate to the shared state the state is ready in the derived class
            ///
            /// This operation marks the ready_ flags and warns any future waiting on it.
            /// This overload is meant to be used by derived classes that might need to use another mutex for this operation
            void mark_ready_and_notify(std::unique_lock<std::mutex> &lk) noexcept {
                assert(lk.owns_lock());
                ready_ = true;
                lk.unlock();
                waiters_.notify_all();
            }

            /// \brief Indicate to the shared state its owner has been destroyed
            ///
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
            ///
            /// This sets the exception value and marks the shared state as ready.
            /// If we try to set an exception on a shared state that's ready, we throw an exception.
            /// This overload is meant to be used by derived classes that might need to use another mutex for this operation
            void set_exception(std::exception_ptr except, // NOLINT(performance-unnecessary-value-param)
                               std::unique_lock<std::mutex> &lk) {
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
                if (waiters_.wait_until(lk, timeout_time, [this]() { return ready_; })) {
                    return std::future_status::ready;
                } else {
                    return std::future_status::timeout;
                }
            }

            /// \brief Check if state is ready without locking
            constexpr bool is_ready_unsafe() const { return ready_; }

            /// \brief Check if state is ready without locking
            bool stores_exception_unsafe() const { return except_ != nullptr; }

            void throw_internal_exception() const { std::rethrow_exception(except_); }

            /// \brief Check if state is ready without locking
            ///
            /// Although the parent shared state class doesn't store the state, we know
            /// it's ready with state because it's the only way it's ready unless it has an exception.
            bool stores_state_unsafe() const { return is_ready_unsafe() && (!stores_exception_unsafe()); }

            /// \brief Generate unique lock for the shared state
            ///
            /// This lock can be used for any operations on the state that might need to be protected/
            std::unique_lock<std::mutex> create_unique_lock() const { return std::unique_lock{mutex_}; }

          private:
            /// \brief Default global mutex for shared state operations
            mutable std::mutex mutex_{};

            /// \brief Indicates if the shared state is already set
            bool ready_{false};

            /// \brief Pointer to an exception, when the shared_state has been set to an exception
            std::exception_ptr except_{nullptr};

            /// \brief Condition variable to notify any object waiting for this shared state to be ready
            mutable std::condition_variable waiters_{};
        };

        /// \brief Determine the type we should use to store a shared state internally
        ///
        /// We usually need uninitialized storage for a given type, since the shared state needs to be in control
        /// of constructors and destructors.
        ///
        /// For trivial types, we can directly store the value.
        ///
        /// When the shared state is a reference, we store pointers internally.
        ///
        template <typename R, class Enable = void> struct shared_state_storage {
            using type = std::aligned_storage_t<sizeof(R), alignof(R)>;
        };

        template <typename R> struct shared_state_storage<R, std::enable_if_t<std::is_trivial_v<R>>> {
            using type = R;
        };

        template <typename R> using shared_state_storage_t = typename shared_state_storage<R>::type;
    }

    /// \brief Shared state specialization for regular variables
    ///
    /// This class stores the data for a shared state holding an element of type T
    ///
    /// The data is stored with uninitialized storage because this will only need to be initialized when the
    /// state becomes ready. This ensures the shared state works for all types and avoids wasting operations
    /// on a constructor we might not use.
    ///
    /// However, initialized storage might be more useful and easier to debug for trivial types.
    ///
    template <typename R> class shared_state : public detail::shared_state_base {
      public:
        /// \brief Default construct a shared state
        ///
        /// This will construct the state with uninitialized storage for R
        ///
        shared_state() = default;

        /// \brief Deleted copy constructor
        ///
        /// The copy constructor does not make sense for shared states as they should be shared.
        /// Besides, copying state that might be uninitialized is a bad idea.
        ///
        shared_state(shared_state const &) = delete;

        /// \brief Deleted copy assignment operator
        ///
        /// These functions do not make sense for shared states as they are shared
        shared_state &operator=(shared_state const &) = delete;

        /// \brief Destructor
        ///
        /// Destruct the shared object R manually if the state is ready with a value
        ///
        ~shared_state() override {
            if constexpr (!std::is_trivial_v<R>) {
                if (stores_state_unsafe()) {
                    reinterpret_cast<R *>(std::addressof(storage_))->~R();
                }
            }
        }

        /// \brief Set the value of the shared state to a copy of value
        ///
        /// This function locks the shared state and makes a copy of the value into the storage.
        void set_value(const R& value) {
            auto lk = create_unique_lock();
            set_value(value, lk);
        }

        /// \brief Set the value of the shared state by moving a value
        ///
        /// This function locks the shared state and moves the value to the state
        void set_value(R &&value) {
            auto lk = create_unique_lock();
            set_value(std::move(value), lk);
        }

        /// \brief Get the value of the shared state
        ///
        /// \return Reference to the state as a reference to R
        R &get() {
            auto lk = create_unique_lock();
            return get(lk);
        }

      private:
        /// \brief Set value of the shared state in the storage by copying the value
        ///
        /// \param value The value for the shared state
        /// \param lk Custom mutex lock
        void set_value(R const &value, std::unique_lock<std::mutex> &lk) {
            assert(lk.owns_lock());
            if (is_ready_unsafe()) {
                throw promise_already_satisfied{};
            }
            ::new (static_cast<void *>(std::addressof(storage_))) R(value);
            mark_ready_and_notify(lk);
        }

        /// \brief Set value of the shared state in the storage by moving the value
        ///
        /// \param value The rvalue for the shared state
        /// \param lk Custom mutex lock
        void set_value(R &&value, std::unique_lock<std::mutex> &lk) {
            assert(lk.owns_lock());
            if (is_ready_unsafe()) {
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
            if (stores_exception_unsafe()) {
                throw_internal_exception();
            }
            return *reinterpret_cast<R *>(std::addressof(storage_));
        }

        /// \brief Aligned opaque storage for an element of type R
        ///
        /// This needs to be aligned_storage_t because the value might not be initialized yet
        detail::shared_state_storage_t<R> storage_{};
    };

    /// \brief Shared state specialization for references
    ///
    /// This class stores the data for a shared state holding a reference to an element of type T
    /// These shared states need to store a pointer to R internally
    template <typename R> class shared_state<R &> : public detail::shared_state_base {
      public:
        /// \brief Default construct a shared state
        ///
        /// This will construct the state with uninitialized storage for R
        shared_state() = default;

        /// \brief Deleted copy constructor
        ///
        /// These functions do not make sense for shared states as they are shared
        shared_state(shared_state const &) = delete;

        /// \brief Destructor
        ///
        /// Destruct the shared object R if the state is ready with a value
        ~shared_state() override = default;

        /// \brief Deleted copy assignment
        ///
        /// These functions do not make sense for shared states as they are shared
        shared_state &operator=(shared_state const &) = delete;

        /// \brief Set the value of the shared state by storing the address of value R in its internal state
        ///
        /// Shared states to references internally store a pointer to the value instead fo copying it.
        /// This function locks the shared state and moves the value to the state
        void set_value(R &value) {
            auto lk = create_unique_lock();
            set_value(value, lk);
        }

        /// \brief Get the value of the shared state as a reference to the internal pointer
        /// \return Reference to the state as a reference to R
        R &get() {
            auto lk = create_unique_lock();
            return get(lk);
        }

      private:
        /// \brief Set reference of the shared state to the address of a value R
        /// \param value The value for the shared state
        /// \param lk Custom mutex lock
        void set_value(R &value, std::unique_lock<std::mutex> &lk) {
            assert(lk.owns_lock());
            if (is_ready_unsafe()) {
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
            if (stores_exception_unsafe()) {
                throw_internal_exception();
            }
            return *value_;
        }

        /// \brief Pointer to an element of type R representing the reference
        /// This needs to be aligned_storage_t because the value might not be initialized yet
        R *value_{nullptr};
    };

    /// \brief Shared state specialization for a void shared state
    ///
    /// A void shared state needs to synchronize waiting, but it does not need to store anything.
    template <> class shared_state<void> : public detail::shared_state_base {
      public:
        /// \brief Default construct a shared state
        ///
        /// This will construct the state with uninitialized storage for R
        shared_state() = default;

        /// \brief Deleted copy constructor
        ///
        /// These functions do not make sense for shared states as they are shared
        shared_state(shared_state const &) = delete;

        /// \brief Destructor
        ///
        /// Destruct the shared object R if the state is ready with a value
        ~shared_state() override = default;

        /// \brief Deleted copy assignment
        ///
        /// These functions do not make sense for shared states as they are shared
        shared_state &operator=(shared_state const &) = delete;

        /// \brief Set the value of the void shared state without any input as "not-error"
        ///
        /// This function locks the shared state and moves the value to the state
        void set_value() {
            auto lk = create_unique_lock();
            set_value(lk);
        }

        /// \brief Get the value of the shared state by waiting and checking for exceptions
        ///
        /// \return Reference to the state as a reference to R
        void get() {
            auto lk = create_unique_lock();
            get(lk);
        }

      private:
        /// \brief Set value of the shared state with no inputs as "no-error"
        ///
        /// \param lk Custom mutex lock
        void set_value(std::unique_lock<std::mutex> &lk) {
            assert(lk.owns_lock());
            if (is_ready_unsafe()) {
                throw promise_already_satisfied();
            }
            mark_ready_and_notify(lk);
        }

        /// \brief Get the value of the shared state by waiting and checking for exceptions
        void get(std::unique_lock<std::mutex> &lk) const {
            assert(lk.owns_lock());
            wait(lk);
            if (stores_exception_unsafe()) {
                throw_internal_exception();
            }
        }
    };

    /** @} */ // \addtogroup futures Futures
} // namespace futures::detail

#endif // FUTURES_SHARED_STATE_H
