//
// Copyright (c) 2021 alandefreitas (alandefreitas@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//

#ifndef FUTURES_FUTURES_DETAIL_SHARED_STATE_HPP
#define FUTURES_FUTURES_DETAIL_SHARED_STATE_HPP

#include <futures/detail/container/small_vector.hpp>
#include <futures/detail/thread/relocker.hpp>
#include <futures/futures/future_error.hpp>
#include <futures/futures/future_options.hpp>
#include <futures/futures/detail/shared_state_storage.hpp>
#include <futures/futures/detail/traits/is_in_args.hpp>
#include <atomic>
#include <future>
#include <condition_variable>

namespace futures::detail {
    /** \addtogroup futures Futures
     *  @{
     */
    /// \brief Members functions and objects common to all shared state
    /// object types (bool ready and exception ptr)
    ///
    /// Shared states for asynchronous operations contain an element of a
    /// given type and an exception.
    ///
    /// All objects such as futures and promises have shared states and
    /// inherit from this class to synchronize their access to their common
    /// shared state.
    template <bool is_always_deferred>
    class shared_state_base
    {
    private:
        /// \brief A list of waiters: condition variables to notify any
        /// object waiting for this shared state to be ready
        using waiter_list = detail::small_vector<std::condition_variable_any *>;

    public:
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
            if constexpr (!is_always_deferred) {
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
            } else {
                status prev = std::exchange(status_, status::ready);
                if (prev == status::waiting) {
                    auto lk = create_wait_lock();
                    waiter_.notify_all();
                    for (auto &&external_waiter: external_waiters_) {
                        external_waiter->notify_all();
                    }
                }
            }
        }

        /// \brief Check if state is ready
        ///
        /// This overload uses the default global mutex for synchronization
        bool
        is_ready() const {
            if constexpr (!is_always_deferred) {
                return status_.load(std::memory_order_acquire) == status::ready;
            } else {
                return status_ == status::ready;
            }
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
            if (is_ready()) {
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
            if (!is_ready()) {
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
            if (!is_ready()) {
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
        /// This function uses the condition variable waiters to wait for
        /// this shared state to be marked as ready.
        ///
        /// Atomic operations are used to ensure we only involve the
        /// waiting lock if the shared state is not ready yet.
        ///
        void
        wait() {
            status prev = [this]() {
                if constexpr (!is_always_deferred) {
                    status v = status::initial;
                    status_.compare_exchange_strong(v, status::waiting);
                    return v;
                } else {
                    status v = status_;
                    if (status_ == status::initial) {
                        status_ = status::waiting;
                    }
                    return v;
                }
            }();
            if (prev != status::ready) {
                run_wait_callback();
                if (prev == status::initial) {
                    this->post_deferred();
                }
                auto lk = create_wait_lock();
                waiter_.wait(lk, [this]() { return is_ready(); });
            }
        }

        /// \brief Wait for the shared state to become ready
        ///
        /// This function uses the condition variable waiters to wait for
        /// this shared state to be marked as ready for a specified
        /// duration. This overload is meant to be used by derived classes
        /// that might need to use another mutex for this operation
        ///
        /// \tparam Rep An arithmetic type representing the number of ticks
        /// \tparam Period A std::ratio representing the tick period
        /// \param timeout_duration maximum duration to block for
        /// \return The state of the shared value
        template <typename Rep, typename Period>
        std::future_status
        wait_for(std::chrono::duration<Rep, Period> const &timeout_duration) {
            status prev = [this]() {
                if constexpr (!is_always_deferred) {
                    status v = status::initial;
                    status_.compare_exchange_strong(v, status::waiting);
                    return v;
                } else {
                    status v = status_;
                    if (status_ == status::initial) {
                        status_ = status::waiting;
                    }
                    return v;
                }
            }();
            if (prev != status::ready) {
                run_wait_callback();
                if (prev == status::initial) {
                    this->post_deferred();
                }
                auto lk = create_wait_lock();
                if (waiter_.wait_for(lk, timeout_duration, [this]() {
                        return is_ready();
                    })) {
                    return std::future_status::ready;
                } else {
                    return std::future_status::timeout;
                }
            }
            return std::future_status::ready;
        }

        /// \brief Wait for the shared state to become ready
        ///
        /// This function uses the condition variable waiters
        /// to wait for this shared state to be marked as ready until a
        /// specified time point. This overload is meant to be used by
        /// derived classes that might need to use another mutex for this
        /// operation
        ///
        /// \tparam Clock The clock type
        /// \tparam Duration The duration type
        /// \param timeout_time maximum time point to block until
        /// \return The state of the shared value
        template <typename Clock, typename Duration>
        std::future_status
        wait_until(
            std::chrono::time_point<Clock, Duration> const &timeout_time) {
            status prev = [this]() {
                if constexpr (!is_always_deferred) {
                    status v = status::initial;
                    status_.compare_exchange_strong(v, status::waiting);
                    return v;
                } else {
                    status v = status_;
                    if (status_ == status::initial) {
                        status_ = status::waiting;
                    }
                    return v;
                }
            }();
            if (prev != status::ready) {
                run_wait_callback();
                if (prev == status::initial) {
                    this->post_deferred();
                }
                auto lk = create_wait_lock();
                if (waiter_.wait_until(lk, timeout_time, [this]() {
                        return is_ready();
                    })) {
                    return std::future_status::ready;
                } else {
                    return std::future_status::timeout;
                }
            }
            return std::future_status::ready;
        }

        /// \brief Include a condition variable in the list of waiters we
        /// need to notify when the state is ready
        ///
        /// \param cv Reference to an external condition variable
        ///
        /// \return Handle which can be used to unnotify when ready
        notify_when_ready_handle
        notify_when_ready(std::condition_variable_any &cv) {
            status prev = [this]() {
                if constexpr (!is_always_deferred) {
                    status v = status::initial;
                    status_.compare_exchange_strong(v, status::waiting);
                    return v;
                } else {
                    status v = status_;
                    if (status_ == status::initial) {
                        status_ = status::waiting;
                    }
                    return v;
                }
            }();
            if (prev == status::initial) {
                this->post_deferred();
            }
            run_wait_callback();
            return external_waiters_.insert(external_waiters_.end(), &cv);
        }

        /// \brief Remove condition variable from list of condition
        /// variables we need to warn about this state
        ///
        /// \param it External condition variable
        void
        unnotify_when_ready(notify_when_ready_handle it) {
            auto lk = create_wait_lock();
            external_waiters_.erase(it);
        }

        template <typename F>
        void
        set_wait_callback(F f) {
            auto lk = create_wait_lock();
            wait_callback_ = f;
        }

        virtual void
        post_deferred(){};

        /// \brief Get a reference to the mutex in the shared state
        std::mutex &
        waiters_mutex() {
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
        /// \brief Call the internal wait callback function
        void
        run_wait_callback() const {
            if (wait_callback_) {
                status prev = [this]() {
                    if constexpr (!is_always_deferred) {
                        return status_.load(std::memory_order_acquire);
                    } else {
                        return status_;
                    }
                }();
                if (prev != status::ready) {
                    auto lk = create_wait_lock();
                    std::function<void()> local_callback(
                        std::move(wait_callback_));
                    relocker relock(lk);
                    local_callback();
                }
            }
        }

        /// \brief The current status of this shared state
        enum status : uint8_t
        {
            /// Nothing happened yet
            initial,
            /// Someone is waiting for the result
            waiting,
            /// The state has been set and we notified everyone
            ready,
        };

        /// \brief Indicates if the shared state is already set
        ///
        /// There are three states possible: nothing, waiting, ready
        mutable std::
            conditional_t<is_always_deferred, status, std::atomic<status>>
                status_{ status::initial };

        /// \brief Pointer to an exception, when the shared_state fails
        ///
        /// std::exception_ptr does not need to be atomic because
        /// the status variable is guarding it.
        ///
        std::exception_ptr except_{ nullptr };

        /// \brief Callback function we should call before waiting for the
        /// shared state
        std::function<void()> wait_callback_;

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

    /// \brief Shared state with its concrete storage
    ///
    /// This class stores the data for a shared state holding an element of
    /// type `R`, which might be a concrete type, a reference, or `void`.
    ///
    /// For most types, the data is stored as uninitialized storage because
    /// this will only need to be initialized when the state becomes ready.
    /// This ensures the shared state works for all types and avoids
    /// wasting operations on a constructor we might not use.
    ///
    /// However, initialized storage is used for trivial types because
    /// this involves no performance penalty. This also shared states
    /// easier to debug.
    ///
    /// If the shared state returns a reference, the data is stored
    /// internally as a pointer.
    ///
    /// A void shared state needs to synchronize waiting, but it does not
    /// need to store anything.
    ///
    /// The storage also uses empty base optimization, which means it
    /// takes no memory if the state is an empty type, such as `void` or
    /// std::allocator.
    ///
    template <class R, class Options>
    class shared_state
        : public shared_state_base<Options::is_always_deferred>
        , public std::enable_shared_from_this<shared_state<R, Options>>
        , private shared_state_storage<R>
        , private conditional_base<
              Options::has_executor,
              typename Options::executor_t,
              0>
        , private conditional_base<
              Options::is_continuable,
              continuations_source<Options::is_always_deferred>,
              1>
        , private conditional_base<Options::is_stoppable, stop_source, 2>
    {
    protected:
        /// \brief Executor storage
        using executor_base = conditional_base<
            Options::has_executor,
            typename Options::executor_t,
            0>;

        using executor_type = typename executor_base::value_type;

        /// \brief Continuations storage
        using continuations_base = conditional_base<
            Options::is_continuable,
            continuations_source<Options::is_always_deferred>,
            1>;

        using continuations_type = typename continuations_base::value_type;

        /// \brief Stop source storage
        using stop_source_base
            = conditional_base<Options::is_stoppable, stop_source, 2>;

        using stop_source_type = typename stop_source_base::value_type;

        using stop_token_type = std::conditional_t<
            Options::is_stoppable,
            stop_token,
            detail::empty_value_type>;

    public:
        /// \brief Destructor
        ///
        /// We might need to destroy the shared object R if the state is
        /// ready with a value.
        ///
        ~shared_state() override {
            if (this->succeeded()) {
                shared_state_storage<R>::destroy();
            }
        }

        /// \brief Constructor
        ///
        /// This function will construct the state with storage for R.
        ///
        shared_state() = default;

        /// \brief Constructor for state with reference to executor
        ///
        /// The executor allows us to emplace continuations on the
        /// same executor by default.
        ///
        explicit shared_state(const executor_type &ex) : executor_base(ex) {}

        /// \brief Deleted copy constructor
        ///
        /// The copy constructor does not make sense for shared states as
        /// they are meant to be shared in all of our use cases with
        /// promises and futures.
        ///
        shared_state(shared_state const &) = delete;

        /// \brief Move constructor
        ///
        shared_state(shared_state &&) noexcept = default;

        /// \brief Move constructor from shared state with different options
        ///
        /// \param other Other shared state
        template <class OtherOptions>
        shared_state(shared_state<R, OtherOptions> &&other) noexcept
            : shared_state_base<Options::is_always_deferred>(std::move(other)),
              shared_state_storage<R>(std::move(other)),
              continuations_base(std::move(other)),
              stop_source_base(std::move(other)) {}

        /// \brief Deleted copy assignment operator
        ///
        /// These functions do not make sense for shared states as they are
        /// meant to be shared.
        ///
        shared_state &
        operator=(shared_state const &)
            = delete;

        /// \brief Move assignment operator
        ///
        /// These functions do not make sense for shared states as they are
        /// meant to be shared.
        ///
        shared_state &
        operator=(shared_state &&)
            = default;

        /// \brief Set the value of the shared state to a copy of value
        ///
        /// This function locks the shared state and makes a copy of the
        /// value into the storage.
        ///
        /// Shared states to references internally store a pointer to the
        /// value instead fo copying it. This function locks the shared
        /// state and moves the value to the state
        ///
        /// This function is unsynchronized and should only be invoked once.
        ///
        /// \param value New state value
        template <class... Args>
        void
        set_value(Args &&...args) {
            if (this->is_ready()) {
                detail::throw_exception<promise_already_satisfied>();
            }
            shared_state_storage<R>::set_value(std::forward<Args>(args)...);
            this->set_ready();
        }

        /// \brief Set value with a callable and an argument list
        template <typename Fn, typename... Args>
        void
        apply(Fn &&fn, Args &&...args) {
            // Use shared_from_this to ensure the state lives after the
            // the function is executed. Some futures might wait()
            // until the shared state value is set but attempt to
            // destroy the state before the continuations and related
            // tasks are executed.
            auto state = this->shared_from_this();
            try {
                if constexpr (!Options::is_stoppable) {
                    if constexpr (std::is_void_v<R>) {
                        std::invoke(
                            std::forward<Fn>(fn),
                            std::forward<Args>(args)...);
                        state->set_value();
                    } else {
                        state->set_value(std::invoke(
                            std::forward<Fn>(fn),
                            std::forward<Args>(args)...));
                    }
                } else {
                    if constexpr (std::is_void_v<R>) {
                        std::invoke(
                            std::forward<Fn>(fn),
                            this->get_stop_token(),
                            std::forward<Args>(args)...);
                        state->set_value();
                    } else {
                        state->set_value(std::invoke(
                            std::forward<Fn>(fn),
                            this->get_stop_token(),
                            std::forward<Args>(args)...));
                    }
                }
            }
            catch (...) {
                state->set_exception(std::current_exception());
            }
            if constexpr (Options::is_continuable) {
                // The state might have been destroyed by the executor
                // by now. Use another shared reference to the
                // continuations.
                state->get_continuations_source().request_run();
            }
        }

        /// \brief Set value with a callable and a tuple or arguments
        template <typename Fn, typename Tuple>
        void
        apply_tuple(Fn &&fn, Tuple &&targs) {
            return apply_tuple_impl(
                std::forward<Fn>(fn),
                std::forward<Tuple>(targs),
                std::make_index_sequence<
                    std::tuple_size_v<std::remove_reference_t<Tuple>>>{});
        }

        /// \brief Get the value of the shared state
        ///
        /// This function waits for the shared state to become ready and
        /// returns its value.
        ///
        /// This function return `R&` unless this is a shared state to
        /// `void`.
        ///
        /// \return Reference to the state as a reference to R
        std::add_lvalue_reference_t<R>
        get() {
            this->wait();
            if (this->failed()) {
                this->throw_internal_exception();
            }
            return shared_state_storage<R>::get();
        }

        const executor_type &
        get_executor() const noexcept {
            static_assert(Options::has_executor);
            return executor_base::get();
        }

        stop_token
        get_stop_token() const noexcept {
            static_assert(Options::is_stoppable);
            return stop_source_base::get().get_token();
        }

        stop_source &
        get_stop_source() const noexcept {
            static_assert(Options::is_stoppable);
            return stop_source_base::get();
        }

        stop_source &
        get_stop_source() noexcept {
            static_assert(Options::is_stoppable);
            return stop_source_base::get();
        }

        typename continuations_base::value_type &
        get_continuations_source() const noexcept {
            static_assert(Options::is_continuable);
            return continuations_base::get();
        }

        typename continuations_base::value_type &
        get_continuations_source() noexcept {
            static_assert(Options::is_continuable);
            return continuations_base::get();
        }

    private:
        template <typename Fn, typename Tuple, std::size_t... I>
        void
        apply_tuple_impl(Fn &&fn, Tuple &&targs, std::index_sequence<I...>) {
            return apply(
                std::forward<Fn>(fn),
                std::get<I>(std::forward<Tuple>(targs))...);
        }
    };

    /// \brief An extension of shared state with storage for a deferred task
    ///
    /// This class provides the same functionality as shared state and
    /// extra storage for a deferred task.
    ///
    /// Because futures keep references to a shared state, what this
    /// effectively achieves is type erasing the task type. Otherwise,
    /// we would need embed the task types in the futures.
    ///
    /// For instance, this would make it impossible to have vectors of
    /// futures without wrapping tasks into a more limited std::function
    /// before for erasing the task type.
    ///
    template <class R, class Options, class Fn, class... Args>
    class deferred_shared_state
        : public shared_state<R, Options>
        , private maybe_empty<Fn, 3>
        , private maybe_empty<std::tuple<std::decay_t<Args>...>, 4>
    {
    private:
        /// \brief Deferred function storage
        using deferred_base = maybe_empty<Fn, 3>;
        using deferred_type = typename deferred_base::value_type;

        /// \brief Deferred Args storage
        using deferred_args_base
            = maybe_empty<std::tuple<std::decay_t<Args>...>, 4>;
        using deferred_args_type = typename deferred_args_base::value_type;

    public:
        /// \brief Destructor
        ///
        /// We might need to destroy the shared object R if the state is
        /// ready with a value.
        ///
        ~deferred_shared_state() override = default;

        /// \brief Constructor
        ///
        /// This function will construct the state with storage for R.
        ///
        deferred_shared_state() = default;

        /// \brief Constructor for deferred state
        ///
        /// The function accepts other function and args types so
        /// (i) we can forward the variables, and (ii) allow compatible
        /// types to be used here. Most of the time, these will be the
        /// same types as Fn and Args.
        template <class OtherFn, class... OtherArgs>
        explicit deferred_shared_state(
            const typename shared_state<R, Options>::executor_type &ex,
            OtherFn &&f,
            OtherArgs &&...args)
            : shared_state<R, Options>(ex),
              deferred_base(std::forward<OtherFn>(f)),
              deferred_args_base(std::forward<OtherArgs>(args)...) {}

        /// \brief Deleted copy constructor
        ///
        /// The copy constructor does not make sense for shared states as
        /// they are meant to be shared in all of our use cases with
        /// promises and futures.
        ///
        deferred_shared_state(deferred_shared_state const &) = delete;

        /// \brief Move constructor
        ///
        deferred_shared_state(deferred_shared_state &&) noexcept = default;

        /// \brief Move constructor from shared state with different options
        ///
        /// \param other Other shared state
        template <class OtherOptions>
        deferred_shared_state(
            deferred_shared_state<R, OtherOptions, Fn, Args...> &&other) noexcept
            : shared_state<R, Options>(std::move(other)),
              deferred_base(std::move(other)),
              deferred_args_base(std::move(other)) {}

        /// \brief Deleted copy assignment operator
        ///
        /// These functions do not make sense for shared states as they are
        /// meant to be shared.
        ///
        deferred_shared_state &
        operator=(deferred_shared_state const &)
            = delete;

        /// \brief Move assignment operator
        ///
        /// These functions do not make sense for shared states as they are
        /// meant to be shared.
        ///
        deferred_shared_state &
        operator=(deferred_shared_state &&)
            = default;

        void
        post_deferred() override {
            // if this is a continuation, wait for tasks that come before
            if constexpr (Options::is_always_deferred) {
                if constexpr (Options::has_executor) {
                    asio::post(
                        shared_state<R, Options>::get_executor(),
                        [this]() {
                        // use shared_from_this to ensure the state lives while
                        // the deferred function is executed. Some futures might
                        // wait() until the shared state value is set but
                        // destroy the state before the continuations and
                        // related tasks are executed.
                        this->apply_tuple(
                            std::move(this->deferred_base::get()),
                            std::move(this->deferred_args_base::get()));
                        });
                } else {
                    apply_tuple(
                        std::move(deferred_base::get()),
                        std::move(deferred_args_base::get()));
                }
            }
        }
    };

    /** @} */ // \addtogroup futures Futures
} // namespace futures::detail

#endif // FUTURES_FUTURES_DETAIL_SHARED_STATE_HPP
