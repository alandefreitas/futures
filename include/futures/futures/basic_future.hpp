//
// Copyright (c) 2021 alandefreitas (alandefreitas@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//

#ifndef FUTURES_FUTURES_BASIC_FUTURE_HPP
#define FUTURES_FUTURES_BASIC_FUTURE_HPP

#include <futures/executor/default_executor.hpp>
#include <futures/futures/future_options.hpp>
#include <futures/futures/stop_token.hpp>
#include <futures/futures/traits/is_future.hpp>
#include <futures/adaptor/detail/make_continuation_state.hpp>
#include <futures/adaptor/detail/unwrap_and_continue.hpp>
#include <futures/detail/exception/throw_exception.hpp>
#include <futures/detail/utility/maybe_copyable.hpp>
#include <futures/detail/utility/maybe_empty.hpp>
#include <futures/futures/detail/continuations_source.hpp>
#include <futures/futures/detail/future_state.hpp>
#include <futures/futures/detail/share_if_not_shared.hpp>
#include <futures/futures/detail/traits/append_future_option.hpp>
#include <futures/futures/detail/traits/is_executor_then_function.hpp>
#include <futures/futures/detail/traits/is_type_template_in_args.hpp>
#include <futures/futures/detail/traits/remove_future_option.hpp>
#include <functional>
#include <utility>
#include <shared_mutex>

namespace futures {
    /** @addtogroup futures Futures
     *
     * \brief Basic future types and functions
     *
     * The futures library provides components to create and launch futures,
     * objects representing data that might not be available yet.
     *  @{
     */

    /** @addtogroup future-types Future types
     *
     * \brief Basic future types
     *
     * This module defines the @ref basic_future template class, which can be
     * used to define futures with a number of extensions.
     *
     *  @{
     */

#ifndef FUTURES_DOXYGEN
    // Fwd-declare basic_future friends
    namespace detail {
        struct async_future_scheduler;
        struct internal_then_functor;
        class waiter_for_any;
        struct make_ready_future_impl;
    } // namespace detail
    template <class R, class Options>
    class promise_base;
    template <class U, class Options>
    class promise;
    template <class Signature, class Options>
    class packaged_task;
#endif

    /// A basic future type with custom features
    /**
     * Note that these classes only provide the capabilities of tracking these
     * features, such as continuations.
     *
     * Setting up these capabilities (creating tokens or setting main future to
     * run continuations) needs to be done when the future is created in a
     * function such as @ref async by creating the appropriate state for each
     * feature.
     *
     * All this behavior is already encapsulated in the @ref async function.
     *
     * @tparam R Type of element
     * @tparam Options Future value options
     *
     */
    template <class R, class Options = future_options<>>
    class basic_future : private detail::maybe_copyable<Options::is_shared>
    {
    private:
        /**
         * Private types
         */

        /// The traits for the shared state
        /**
         * We remove shared_opt from operation state traits because there's
         * no difference in their internal implementation. The difference
         * is in the future, which needs to choose between inline storage or
         * storage through a shared pointer.
         */
        using operation_state_options = detail::
            remove_future_option_t<shared_opt, Options>;
        static_assert(
            !operation_state_options::is_shared,
            "The underlying operation state cannot be shared");

        /// Determine type of inline operation state
        /**
         * A future that is always deferred needs to store the function type
         * beyond the state type. The extended operation state with information
         * about the function type is implemented through the deferred operation
         * state class, where the function type is not erased.
         *
         * @tparam is_always_deferred whether the future is always deferred
         */
        template <bool is_always_deferred>
        struct operation_state_type_impl
        {
            using type = std::conditional_t<
                is_always_deferred,
                // Type with complete information about the task
                detail::deferred_operation_state<R, operation_state_options>,
                // Type to be shared, task is erased
                detail::operation_state<R, operation_state_options>>;
        };

        static constexpr bool inline_op_state = Options::is_always_deferred
                                                && !Options::is_shared;

        using operation_state_type = typename operation_state_type_impl<
            inline_op_state>::type;

        /// Determine the shared state type, whenever it needs to be shared
        using shared_state_type = std::shared_ptr<operation_state_type>;

        /// Determine the future state type
        /**
         * The future state is a variant type the might hold no state, inline
         * value storage, an operation state, or a shared state.
         *
         * The most efficient alternative will be chosen according to how
         * the future is constructed.
         */
        using future_state_type = detail::future_state<R, operation_state_type>;


        /// Determine the base operation state type
        /**
         * The base operation state implementation type is where the
         * notification handle lives, so we can also expose it in the future
         * type.
         */
        using operation_state_base = detail::operation_state_base<
            Options::is_always_deferred>;

        /// Determine the type of notification handles
        /**
         * Notification handles are used to allow external condition variables
         * to wait for a future value.
         */
        using notify_when_ready_handle = typename operation_state_base::
            notify_when_ready_handle;

        /// Determine the type of this basic future when shared
        using shared_basic_future = basic_future<
            R,
            detail::append_future_option_t<shared_opt, Options>>;

        /**
         * @}
         */

        /**
         * @name Friend types
         *
         * Future types need to be created through other function available
         * for launching futures. These friend types have access to the
         * private basic_future constructors to achieve that.
         *
         * @{
         */

        // Convert similar future types
        template <class U, class O>
        friend class basic_future;

        // Share shared state with the future
        template <class U, class O>
        friend class ::futures::promise_base;

        template <class U, class O>
        friend class ::futures::promise;

        template <typename Signature, class Opts>
        friend class ::futures::packaged_task;

        // Launch futures
        friend struct detail::async_future_scheduler;

        // Launch future continuations
        friend struct detail::internal_then_functor;

        // Append external waiters to future
        friend class detail::waiter_for_any;

        // Append external waiters to future
        friend struct detail::make_ready_future_impl;

        /**
         * @}
         */

        /**
         * @name Private constructors
         *
         * Futures should be constructed through friend functions, which will
         * create a shared state for the task they launch and the future.
         *
         * @{
         */

        /// Construct future from a shared operation state
        /**
         * This constructor is used by @ref async and other launching functions
         * that create eager futures. It represents the traditional type of
         * shared state, also available in C++11 std::future.
         *
         * These eager functions initialize the future with a shared state
         * because the task needs to know the address of this state in order
         * to properly set the value of the operation state. This means these
         * functions require memory allocation, which we later optimize with
         * appropriate allocators for the shared states.
         *
         * This constructor is private because we need to ensure the launching
         * functions appropriately set the future handling these traits, such
         * as continuations.
         *
         * @param s Future shared state
         */
        explicit basic_future(const shared_state_type &s) noexcept
            : detail::maybe_copyable<Options::is_shared>{}, state_{ s } {}

        /// Construct future from an rvalue shared operation state
        /**
         * This constructor is used by @ref async and other launching functions
         * that create eager futures. It represents the traditional type of
         * shared state, also available in C++11 std::future.
         *
         * These eager functions initialize the future with a shared state
         * because the task needs to know the address of this state in order
         * to properly set the value of the operation state. This means these
         * functions require memory allocation, which we later optimize with
         * appropriate allocators for the shared states.
         *
         * This constructor is private because we need to ensure the launching
         * functions appropriately set the future handling these traits, such
         * as continuations.
         *
         * @param s Future shared state
         */
        explicit basic_future(shared_state_type &&s) noexcept
            : detail::maybe_copyable<Options::is_shared>{},
              state_{ std::move(s) } {}

        /// Construct from an inline operation state
        /**
         * This constructor is mostly used by @ref schedule and other scheduling
         * functions that create deferred futures. It represents an operation
         * state that is not-shared, which makes it more efficient in terms
         * of memory allocations.
         *
         * These deferred functions initialize the future with an inline
         * operation state the task will only be launched later and its
         * address cannot change once we wait for the future, because the
         * calling thread will be blocked and the state address cannot change.
         *
         * However, there are circumstances where a deferred future will need
         * to convert this into a shared state. The first situation is when
         * we `wait_for` or `wait_until` on a deferred future. This means the
         * address of an inline state could change after we wait for a while,
         * thus launching the task, and the future is moved afterwards. A
         * second situation where the deferred state might need to be shared
         * is, naturally, with shared futures.
         *
         * This constructor is private because we need to ensure the scheduling
         * functions appropriately set the future handling these traits, such
         * as continuations.
         *
         * @param op Future inline operation state
         */
        explicit basic_future(operation_state_type &&op) noexcept
            : detail::maybe_copyable<Options::is_shared>{},
              state_{ std::move(op) } {}

        /// Construct from an direct operation state storage
        /**
         * This constructor is used by @ref make_ready_future and other
         * functions that create futures objects holding values that might be
         * already known at construction. These future objects are often
         * required in algorithms that involve future values to interoperate
         * with known values using the same type for both.
         *
         * It represents an operation state storage that is not-shared, which
         * makes there future objects more efficient than others, instead of
         * emulating known values through memory allocations.
         *
         * @param op Future inline operation state
         */
        explicit basic_future(detail::operation_state_storage<R> &&op) noexcept
            : detail::maybe_copyable<Options::is_shared>{},
              state_{ std::move(op) } {}

        /// Construct future from a variant future operation state
        /**
         * This constructor is used by any function that might create a future
         * from another future, where the variant state is already constructed.
         *
         * This function might be used by eager and deferred futures.
         *
         * @param s Future shared state
         */
        explicit basic_future(const future_state_type &s) noexcept
            : detail::maybe_copyable<Options::is_shared>{}, state_{ s } {}

        /// Construct future from an rvalue variant future operation state
        /**
         * This constructor is used by any function that might create a future
         * from another future, where the variant state is already constructed.
         *
         * This function might be used by eager and deferred futures.
         *
         * @param s Future shared state
         */
        explicit basic_future(future_state_type &&s) noexcept
            : detail::maybe_copyable<Options::is_shared>{},
              state_{ std::move(s) } {}

        /// Construct a ready future from a value_type
        /**
         * This constructor is used by any function that might create a future
         * that's already ready.
         *
         * This function might be used by eager and deferred futures.
         *
         * @param v Future value
         */
        template <
            class T
#ifndef FUTURES_DOXYGEN
            ,
            std::enable_if_t<std::is_same_v<T, R> && !std::is_void_v<R>, int> = 0
#endif
            >
        explicit basic_future(T &&v) noexcept
            : detail::maybe_copyable<Options::is_shared>{}, state_{
                  detail::in_place_type_t<detail::operation_state_storage<R>>{},
                  std::forward<T>(v)
              } {
        }

        /**
         * @}
         */
    public:
        /// @name Public types
        /// @{

        using value_type = R;

        /// @}

        /// @name Constructors
        /// @{

        /// Destructor
        ~basic_future() {
            if constexpr (Options::is_stoppable && !Options::is_shared) {
                if (valid() && !is_ready()) {
                    get_stop_source().request_stop();
                }
            }
            wait_if_last();
        }

        /// Constructor
        /**
         * The default constructor creates an invalid future with no shared
         * state.
         *
         * After construction, `valid() == false`.
         */
        basic_future() noexcept = default;

        /// Copy constructor
        /**
         * Constructs a shared future that refers to the same shared state,
         * if any, as other.
         *
         * @note The copy constructor only participates in overload resolution
         * if `basic_future` is shared.
         */
        basic_future(const basic_future &other) = default;

        /// Copy assignment
        /**
         * Constructs a shared future that refers to the same shared state,
         * if any, as other.
         *
         * @note The copy assignment only participates in overload resolution
         * if `basic_future` is shared.
         */
        basic_future &
        operator=(const basic_future &other)
            = default;

        /// Move constructor.
        /**
         * Constructs a basic_future with the operation state of other using
         * move semantics.
         *
         * After construction, other.valid() == false.
         */
        basic_future(basic_future &&other) noexcept
            : detail::maybe_copyable<Options::is_shared>{},
              state_{ std::move(other.state_) },
              join_{ std::exchange(other.join_, false) } {}

        /// Move assignment.
        /**
         * Constructs a basic_future with the operation state of other using
         * move semantics.
         *
         * After construction, other.valid() == false.
         */
        basic_future &
        operator=(basic_future &&other) noexcept {
            state_ = std::move(other.state_);
            join_ = std::exchange(other.join_, false);
            return *this;
        }

        /// @}

        /**
         * @name Sharing
         * @{
         */

        /// Create another future whose state is shared
        /**
         * Create a shared variant of the current future object.
         *
         * If the current type is not shared, the object becomes invalid.
         *
         * If the current type is already shared, the new object is equivalent
         * to a copy.
         *
         * @return A shared variant of this future
         */
        basic_future<R, detail::append_future_option_t<shared_opt, Options>>
        share() {
            if (!valid()) {
                detail::throw_exception<future_uninitialized>();
            }

            // Determine type of corresponding shared future
            using shared_options = detail::
                append_future_option_t<shared_opt, Options>;
            using shared_future_t = basic_future<R, shared_options>;

            // Create future state for the shared future
            if constexpr (Options::is_shared) {
                shared_future_t other{ state_ };
                other.join_ = join_;
                return other;
            } else {
                shared_future_t other{ std::move(state_) };
                other.join_ = std::exchange(join_, false);
                return other;
            }
        }

        /**
         * @}
         */

        /**
         * @name Getting the result
         * @{
         */

        /// Returns the result
        /**
         * The get member function waits until the future has a valid result
         * and retrieves it.
         *
         * It effectively calls wait() in order to wait for the result.
         *
         * The behavior is undefined if `valid()` is false before the call to
         * this function.
         *
         * If the future is unique, any shared state is released and `valid()`
         * is `false` after a call to this member function.
         *
         * - Unique futures:
         *     - `R` -> return `R`
         *     - `R&` -> return `R&`
         *     - `void` -> return `void`
         * - Shared futures:
         *     - `R` -> return `const R&`
         *     - `R&` -> return `R&`
         *     - `void` -> return `void`
         *
         */
        decltype(auto)
        get() {
            if (!valid()) {
                detail::throw_exception<future_uninitialized>();
            }
            state_.wait();
            if constexpr (Options::is_shared) {
                // state_.get() should handle the return type for us
                return state_.get();
            } else {
                future_state_type tmp(std::move(state_));
                if constexpr (std::is_reference_v<R> || std::is_void_v<R>) {
                    return tmp.get();
                } else {
                    return R(std::move(tmp.get()));
                }
            }
        }

        /// Get exception pointer without throwing an exception
        /**
         * If the future does not hold an exception, the exception_ptr is
         * nullptr.
         *
         * This extends future objects so that we can always check if the future
         * contains an exception without throwing it.
         *
         * @return An exception pointer
         */
        std::exception_ptr
        get_exception_ptr() {
            if (!valid()) {
                detail::throw_exception<future_uninitialized>();
            }
            state_.wait();
            return state_.get_exception_ptr();
        }

        /**
         * @}
         */

        /**
         * @name Future state
         *
         * Observe the current state of the future value
         *
         * @{
         */

        /// Checks if the future refers to a valid operation state
        /**
         * This is the case only for futures that were not default-constructed
         * or moved from until the first time get() or share() is called. If the
         * future is shared, its state is not invalidated when get() is called.
         *
         * If any member function other than the destructor, the move-assignment
         * operator, or `valid` is called on a future that does not refer a
         * valid operation state, a future_error will be throw to indicate
         * `no_state`.
         *
         * It is valid to move (or copy, for shared futures) from a future
         * object for which `valid()` is `false`.
         *
         * @return true if `*this` refers to a valid operation state
         */
        [[nodiscard]] bool
        valid() const {
            return state_.valid();
        }

        /// Waits for the result to become available
        /**
         * Blocks until the result becomes available.
         *
         * `valid() == true` after the call.
         *
         * A `future_uninitialized` exception is thrown if `valid() == false`
         * before the call to this function.
         */
        void
        wait() const {
            if (!valid()) {
                detail::throw_exception<future_uninitialized>();
            }
            if constexpr (Options::is_always_deferred) {
                detail::throw_exception<future_deferred>();
            }
            state_.wait();
        }

        /// Waits for the result to become available
        /**
         * Blocks until the result becomes available.
         *
         * `valid() == true` after the call.
         *
         * A `future_uninitialized` exception is thrown if `valid() == false`
         * before the call to this function.
         */
        void
        wait() {
            if (!valid()) {
                detail::throw_exception<future_uninitialized>();
            }
            state_.wait();
        }

        /// Waits for the result, returns if it is unavailable for duration
        /**
         * Waits for the result to become available. Blocks until specified
         * timeout_duration has elapsed or the result becomes available,
         * whichever comes first. The return value identifies the state of the
         * result.
         *
         * If the future is deferred, the operation state might be converted
         * into a shared operation state. This ensures that (i) the result will
         * be computed only when explicitly requested, and (ii) the address of
         * the operation state will not change once the result is requested.
         *
         * This function may block for longer than `timeout_duration` due to
         * scheduling or resource contention delays.
         *
         * The behavior is undefined if valid() is false before the call to
         * this function.
         *
         * @param timeout_duration maximum duration to block for
         *
         * @return future status
         */
        template <class Rep, class Period>
        std::future_status
        wait_for(
            const std::chrono::duration<Rep, Period> &timeout_duration) const {
            if (!valid()) {
                detail::throw_exception<future_uninitialized>();
            }
            if constexpr (Options::is_always_deferred) {
                return std::future_status::deferred;
            }
            return state_.wait_for(timeout_duration);
        }

        /// Waits for the result, returns if it is unavailable for duration
        template <class Rep, class Period>
        std::future_status
        wait_for(const std::chrono::duration<Rep, Period> &timeout_duration) {
            if (!valid()) {
                detail::throw_exception<future_uninitialized>();
            }
            return state_.wait_for(timeout_duration);
        }

        /// Waits for the result, returns if it is unavailable for duration
        /**
         * Waits for a result to become available. It blocks until specified
         * `timeout_time` has been reached or the result becomes available,
         * whichever comes first. The return value indicates why `wait_until`
         * returned.
         *
         * If the future is deferred, the operation state might be converted
         * into a shared operation state. This ensures that (i) the result will
         * be computed only when explicitly requested, and (ii) the address of
         * the operation state will not change once the result is requested.
         *
         * This function may block until after `timeout_time` due to
         * scheduling or resource contention delays.
         *
         * The behavior is undefined if valid() is false before the call to
         * this function.
         *
         * @param timeout_time maximum time point to block until
         *
         * @return future status
         */
        template <class Clock, class Duration>
        std::future_status
        wait_until(const std::chrono::time_point<Clock, Duration> &timeout_time)
            const {
            if (!valid()) {
                detail::throw_exception<future_uninitialized>();
            }
            return state_.wait_until(timeout_time);
        }

        /// Waits for the result, returns if it is unavailable for duration
        template <class Clock, class Duration>
        std::future_status
        wait_until(
            const std::chrono::time_point<Clock, Duration> &timeout_time) {
            if (!valid()) {
                detail::throw_exception<future_uninitialized>();
            }
            if constexpr (Options::is_always_deferred) {
                detail::throw_exception<future_deferred>();
            }
            return state_.wait_until(timeout_time);
        }

        /// Checks if the associated operation state is ready.
        /**
         * Checks if the associated shared state is ready.
         *
         * The behavior is undefined if valid() is false.
         *
         * @return `true` if the associated shared state is ready
         */
        [[nodiscard]] bool
        is_ready() const {
            if (!valid()) {
                detail::throw_exception<std::future_error>(
                    std::future_errc::no_state);
            }
            return state_.is_ready();
        }

        /// Tell this future not to join at destruction
        /**
         * For safety, all futures wait at destruction by default.
         *
         * This function separates the execution from the future object,
         * allowing execution to continue independently.
         */
        void
        detach() {
            join_ = false;
        }

        /**
         * @}
         */

        /**
         * @name Continuations
         *
         * @{
         */

        /// Attaches a continuation to a future
        /**
         * Attach the continuation function to this future object. The behavior
         * is undefined if this future has no associated operation state
         * (i.e., `valid() == false`).
         *
         * Creates an operation state associated with the future object to be
         * returned.
         *
         * When the shared state currently associated with this future is ready,
         * the continuation is called on the specified executor.
         *
         * Any value returned from the continuation is stored as the result in
         * the operation state of the returned future object. Any exception
         * propagated from the execution of the continuation is stored as the
         * exceptional result in the operation state of the returned future
         * object.
         *
         * A continuation to an eager future is also eager. If this future is
         * eager, the continuation is attached to a list of continuations of
         * this future.
         *
         * A continuation to a deferred future is also deferred. If this future
         * is deferred, this future is stored as the parent future of the next
         * future.
         *
         * If this future is ready, the continuation is directly launched or
         * scheduled in the specified executor.
         *
         * @note Unlike `std::experimental::future`, when the return type of the
         * continuation function is also a future, this function performs no
         * implicit unwrapping on the return type with the `get` function. This
         * (i) simplifies the development of generic algorithms with futures,
         * (ii) makes the executor for the unwrapping task explicit, and (iii)
         * allows the user to retrieve the returned type as a future or as
         * its unwrapped type.
         *
         * @note This function only participates in overload resolution if the
         * future supports continuations
         *
         * @tparam Executor Executor type
         * @tparam Fn Function type
         *
         * @param ex An executor
         * @param fn A continuation function
         *
         * @return The continuation future
         */
        template <
            class Executor,
            class Fn
#ifndef FUTURES_DOXYGEN
            ,
            bool U = (Options::is_continuable || Options::is_always_deferred),
            std::enable_if_t<
                U && U == (Options::is_continuable || Options::is_always_deferred),
                int> = 0
#endif
            >
        decltype(auto)
        then(const Executor &ex, Fn &&fn) {
            // Throw if invalid
            if (!valid()) {
                detail::throw_exception<std::future_error>(
                    std::future_errc::no_state);
            }

            if constexpr (Options::is_continuable) {
                // Determine traits for the next future
                using traits = detail::
                    continuation_traits<Executor, Fn, basic_future>;
                using next_value_type = typename traits::next_value_type;
                using next_future_options = typename traits::next_future_options;
                using next_future_type
                    = basic_future<next_value_type, next_future_options>;

                // Both futures are eager and continuable
                static_assert(!is_always_deferred_v<basic_future>);
                static_assert(!is_always_deferred_v<next_future_type>);
                static_assert(is_continuable_v<basic_future>);
                static_assert(is_continuable_v<next_future_type>);

                // Store a backup of the continuations source
                auto cont_source = get_continuations_source();

                // Create continuation function
                // note: this future is moved into this task
                // note: this future being shared allows this to be copy
                // constructible
                detail::unwrap_and_continue_task<
                    std::decay_t<basic_future>,
                    std::decay_t<Fn>>
                    task{ detail::move_if_not_shared(*this),
                          std::forward<Fn>(fn) };

                // Create a shared operation state for next future
                // note: we use a shared state because the continuation is also
                // eager
                using operation_state_t = detail::
                    operation_state<next_value_type, next_future_options>;
                auto state = std::make_shared<operation_state_t>(ex);
                next_future_type fut(state);

                // Create task to set next future state
                // note: this function might become non-copy-constructible
                // because it stores the continuation function.
                auto set_state_fn =
                    [state = std::move(state),
                     task = std::move(task)]() mutable {
                    state->apply(std::move(task));
                };
                using set_state_fn_type = decltype(set_state_fn);

                // Attach set_state_fn to this continuation list
                if constexpr (std::is_copy_constructible_v<set_state_fn_type>) {
                    cont_source.push(ex, std::move(set_state_fn));
                } else {
                    // Make the continuation task copyable if we have to
                    // note: the continuation source uses `std::function` to
                    // represent continuations.
                    // note: This could be improved with an implementation of
                    // `std::move_only_function` to be used by the continuation
                    // source.
                    auto fn_shared_ptr = std::make_shared<set_state_fn_type>(
                        std::move(set_state_fn));
                    auto copyable_handle = [fn_shared_ptr]() {
                        (*fn_shared_ptr)();
                    };
                    cont_source.push(ex, copyable_handle);
                }
                return fut;
            } else if constexpr (Options::is_always_deferred) {
                // Determine traits for the next future
                using traits = detail::
                    continuation_traits<Executor, Fn, basic_future>;
                using next_value_type = typename traits::next_value_type;
                using next_future_options = typename traits::next_future_options;
                using next_future_type
                    = basic_future<next_value_type, next_future_options>;

                // Both future types are deferred
                static_assert(is_always_deferred_v<basic_future>);
                static_assert(is_always_deferred_v<next_future_type>);

                // Create continuation function
                // note: this future is moved into this task
                // note: this future is not always shared, in which case
                // the operation state is still inline in another address,
                // which is OK because the value hasn't been requested
                detail::unwrap_and_continue_task<
                    std::decay_t<basic_future>,
                    std::decay_t<Fn>>
                    task{ detail::move_if_not_shared(*this),
                          std::forward<Fn>(fn) };

                // Create the operation state for the next future
                // note: this state is inline because the continuation
                // is also deferred
                // note: This operation contains the task
                using deferred_operation_state_t = detail::
                    deferred_operation_state<
                        next_value_type,
                        next_future_options>;
                deferred_operation_state_t state(ex, std::move(task));

                // Move operation state into the new future
                // note: this is the new future representing the deferred
                // task graph now. It has inline access to the parent
                // operation.
                next_future_type fut(std::move(state));
                return fut;
            }
        }

        /// Attaches a continuation to a future on the same executor
        /**
         * Attach the continuation function to this future object with the
         * default executor.
         *
         * When the shared state currently associated with this future is ready,
         * the continuation is called on the same executor as this future. If
         * no executor is associated with this future, the default executor
         * is used.
         *
         * @note This function only participates in overload resolution if the
         * future supports continuations
         *
         * @tparam Fn Function type
         *
         * @param fn A continuation function
         *
         * @return The continuation future
         */
        template <
            class Fn
#ifndef FUTURES_DOXYGEN
            ,
            bool U = Options::is_continuable || Options::is_always_deferred,
            std::enable_if_t<
                U && U == (Options::is_continuable || Options::is_always_deferred),
                int> = 0
#endif
            >
        decltype(auto)
        then(Fn &&fn) {
            if constexpr (Options::has_executor) {
                return this->then(get_executor(), std::forward<Fn>(fn));
            } else {
                return this
                    ->then(make_default_executor(), std::forward<Fn>(fn));
            }
        }

        /// Get the current executor for this task
        /**
         * @note This function only participates in overload resolution
         * if future_options has an associated executor
         *
         * @return The executor associated with this future instance
         */
        template <
#ifndef FUTURES_DOXYGEN
            bool U = Options::has_executor,
            std::enable_if_t<U && U == Options::has_executor, int> = 0
#endif
            >
        const typename Options::executor_t &
        get_executor() const {
            static_assert(Options::has_executor);
            return state_.get_executor();
        }

        /**
         * @}
         */

        /**
         * @name Stop requests
         * @{
         */

        /// Requests execution stop via the shared stop state of the task
        /**
         * Issues a stop request to the internal stop-state, if it has not yet
         * already had stop requested. The task associated to the future
         * can use the stop token to identify it should stop running.
         *
         * The determination is made atomically, and if stop was requested, the
         * stop-state is atomically updated to avoid race conditions.
         *
         * @note This function only participates in overload resolution if the
         * future supports stop tokens
         *
         * @return `true` if this invocation made a stop request
         */
        template <
#ifndef FUTURES_DOXYGEN
            bool U = Options::is_stoppable,
            std::enable_if_t<U && U == Options::is_stoppable, int> = 0
#endif
            >
        bool
        request_stop() noexcept {
            return get_stop_source().request_stop();
        }

        /// Get this future's stop source
        /**
         * Returns a stop_source associated with the same shared stop-state as
         * held internally by the future task object.
         *
         * @note This function only participates in overload resolution if the
         * future supports stop tokens
         *
         * @return The stop source
         */
        template <
#ifndef FUTURES_DOXYGEN
            bool U = Options::is_stoppable,
            std::enable_if_t<U && U == Options::is_stoppable, int> = 0
#endif
            >
        [[nodiscard]] stop_source
        get_stop_source() const noexcept {
            return state_.get_stop_source();
        }

        /// Get this future's stop token
        /**
         * Returns a stop_token associated with the same shared stop-state held
         * internally by the future task object.
         *
         * @note This function only participates in overload resolution if the
         * future supports stop tokens
         *
         * @return The stop token
         */
        template <
#ifndef FUTURES_DOXYGEN
            bool U = Options::is_stoppable,
            std::enable_if_t<U && U == Options::is_stoppable, int> = 0
#endif
            >
        [[nodiscard]] stop_token
        get_stop_token() const noexcept {
            return get_stop_source().get_token();
        }

        /**
         * @}
         */

    private:
        /// @name Private Functions
        /// @{

        /// Get this future's continuations source
        /**
         * @note This function only participates in overload resolution if the
         * future supports continuations
         *
         * @return The continuations source
         */
        template <
#ifndef FUTURES_DOXYGEN
            bool U = Options::is_continuable,
            std::enable_if_t<U && U == Options::is_continuable, int> = 0
#endif
            >
        [[nodiscard]] decltype(auto)
        get_continuations_source() const noexcept {
            return state_.get_continuations_source();
        }

        /// Notify this condition variable when the future is ready
        notify_when_ready_handle
        notify_when_ready(std::condition_variable_any &cv) {
            if (!valid()) {
                detail::throw_exception<future_uninitialized>();
            }
            return state_.notify_when_ready(cv);
        }

        /// Cancel request to notify this condition variable when the
        /// future is ready
        void
        unnotify_when_ready(notify_when_ready_handle h) {
            if (!valid()) {
                detail::throw_exception<future_uninitialized>();
            }
            return state_.unnotify_when_ready(h);
        }

        /// Get a reference to the mutex in the underlying shared state
        std::mutex &
        waiters_mutex() {
            if (!valid()) {
                detail::throw_exception<future_uninitialized>();
            }
            return state_.waiters_mutex();
        }

        /// Wait if this is the last future referring to the operation state
        void
        wait_if_last() {
            if constexpr (Options::is_shared) {
                if (join_ && valid() && !is_ready() && state_.use_count() == 1)
                {
                    wait();
                }
            } else {
                if (join_ && valid() && !is_ready()) {
                    wait();
                }
            }
        }

        /// @}

        /// @name Members
        /// @{
        /// Pointer to shared state
        mutable future_state_type state_{};

        /// Whether this future should join at destruction
        bool join_{ !Options::is_always_detached };
        /// @}
    };

#ifndef FUTURES_DOXYGEN
    /// Define all basic_futures as a kind of future
    template <typename... Args>
    struct is_future<basic_future<Args...>> : std::true_type
    {};

    /// Define all basic_futures as a future with a ready notifier
    template <typename... Args>
    struct has_ready_notifier<basic_future<Args...>> : std::true_type
    {};

    /// Define shared basic_futures as supporting shared values
    template <class T, class... Args>
    struct is_shared_future<
        basic_future<T, detail::future_options_list<Args...>>>
        : detail::is_in_args<shared_opt, Args...>
    {};

    /// Define continuable basic_futures as supporting lazy continuations
    template <class T, class... Args>
    struct is_continuable<basic_future<T, detail::future_options_list<Args...>>>
        : detail::is_in_args<continuable_opt, Args...>
    {};

    /// Define stoppable basic_futures as being stoppable
    template <class T, class... Args>
    struct is_stoppable<basic_future<T, detail::future_options_list<Args...>>>
        : detail::is_in_args<stoppable_opt, Args...>
    {};

    /// Define stoppable basic_futures as having a stop token
    /**
     * Some futures might be stoppable without a stop token
     */
    template <class T, class... Args>
    struct has_stop_token<basic_future<T, detail::future_options_list<Args...>>>
        : detail::is_in_args<stoppable_opt, Args...>
    {};

    /// Define deferred basic_futures as being deferred
    template <class T, class... Args>
    struct is_always_deferred<
        basic_future<T, detail::future_options_list<Args...>>>
        : detail::is_in_args<always_deferred_opt, Args...>
    {};

    /// Define deferred basic_futures as having an executor
    template <class T, class... Args>
    struct has_executor<basic_future<T, detail::future_options_list<Args...>>>
        : detail::is_type_template_in_args<executor_opt, Args...>
    {};
#endif

    /// A simple future type similar to `std::future`
    /**
     * We should only use this future type for eager tasks that do not
     * expect continuations.
     */
    template <class T, class Executor = default_executor_type>
    using future = basic_future<T, future_options<executor_opt<Executor>>>;

    /// A future type with lazy continuations
    /**
     * Futures with lazy continuations contains a list of continuation tasks
     * to be launched once the main task is complete.
     *
     * This is what a @ref futures::async returns by default when the first
     * function parameter is not a @ref futures::stop_token.
     **/
    template <class T, class Executor = default_executor_type>
    using cfuture = basic_future<
        T,
        future_options<executor_opt<Executor>, continuable_opt>>;

    /// A future type with lazy continuations and stop tokens
    /**
     *  It's a quite common use case that we need a way to cancel futures and
     *  jcfuture provides us with an even better way to do that.
     *
     *  This is what @ref futures::async returns when the first function
     *  parameter is a @ref futures::stop_token
     */
    template <class T, class Executor = default_executor_type>
    using jcfuture = basic_future<
        T,
        future_options<executor_opt<Executor>, continuable_opt, stoppable_opt>>;

    /// A deferred future type
    /**
     * This is a future type whose main task will only be launched when we
     * wait for its results from another execution context.
     *
     * This is what the function @ref schedule returns when the first task
     * parameter is not a stop token.
     *
     * The state of this future stores the function to be run.
     *
     * This future type supports continuations without the continuation lists
     * of continuable futures.
     *
     */
    template <class T, class Executor = default_executor_type>
    using dfuture = basic_future<
        T,
        future_options<executor_opt<Executor>, always_deferred_opt>>;

    /// A deferred stoppable future type
    /**
     * This is a future type whose main task will only be launched when we
     * wait for its results from another execution context.
     *
     * Once the task is launched, it can be requested to stop through its
     * stop source.
     *
     * This is what the function @ref schedule returns when the first task
     * parameter is a stop token.
     */
    template <class T, class Executor = default_executor_type>
    using jdfuture = basic_future<
        T,
        future_options<executor_opt<Executor>, always_deferred_opt>>;

    /// A future that simply holds a ready value
    /**
     * This is the future type we use for constant values. This is the
     * future type we usually return from functions such as
     * @ref make_ready_future.
     *
     * These futures have no support for associated executors, continuations,
     * or deferred tasks.
     *
     * Like deferred futures, the operation state is stored inline.
     *
     */
    template <class T>
    using vfuture = basic_future<T, future_options<>>;

    /// A simple std::shared_future
    /**
     * This is what a futures::future::share() returns
     */
    template <class T, class Executor = default_executor_type>
    using shared_future
        = basic_future<T, future_options<executor_opt<Executor>, shared_opt>>;

    /// A shared future type with lazy continuations
    /**
     * This is what a @ref futures::cfuture::share() returns
     */
    template <class T, class Executor = default_executor_type>
    using shared_cfuture = basic_future<
        T,
        future_options<executor_opt<Executor>, continuable_opt, shared_opt>>;

    /// A shared future type with lazy continuations and stop tokens
    /**
     * This is what a @ref futures::jcfuture::share() returns
     */
    template <class T, class Executor = default_executor_type>
    using shared_jcfuture = basic_future<
        T,
        future_options<
            executor_opt<Executor>,
            continuable_opt,
            stoppable_opt,
            shared_opt>>;

    /// A shared future type with deferred task
    /**
     * This is what a @ref futures::dfuture::share() returns
     */
    template <class T, class Executor = default_executor_type>
    using shared_dfuture = basic_future<
        T,
        future_options<executor_opt<Executor>, always_deferred_opt, shared_opt>>;

    /// A shared future type with deferred task and stop token
    /**
     * This is what a @ref futures::jdfuture::share() returns
     */
    template <class T, class Executor = default_executor_type>
    using shared_jdfuture = basic_future<
        T,
        future_options<
            executor_opt<Executor>,
            stoppable_opt,
            always_deferred_opt,
            shared_opt>>;

    /// A shared future that simply holds a ready value
    template <class T>
    using shared_vfuture = basic_future<T, future_options<shared_opt>>;

    /** @} */
    /** @} */
} // namespace futures

#endif // FUTURES_FUTURES_BASIC_FUTURE_HPP
