//
// Copyright (c) 2022 alandefreitas (alandefreitas@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//

#ifndef FUTURES_DETAIL_VARIANT_STATE_HPP
#define FUTURES_DETAIL_VARIANT_STATE_HPP

#include <futures/config.hpp>
#include <futures/throw.hpp>
#include <futures/detail/operation_state.hpp>
#include <futures/detail/operation_state_storage.hpp>
#include <futures/detail/shared_state.hpp>
#include <futures/detail/traits/std_type_traits.hpp>
#include <futures/detail/utility/byte.hpp>
#include <futures/detail/deps/boost/core/empty_value.hpp>
#include <futures/detail/deps/boost/mp11/algorithm.hpp>
#include <futures/detail/deps/boost/variant2/variant.hpp>

namespace futures {
    namespace detail {
        // The variant operation state used in instances of basic_future
        /*
         * This class models an operation state in the various formats it might
         * be found in a future:
         *
         * - Empty state (i.e. default constructed and moved-from futures)
         * - Direct value storage (i.e.: futures created with make_ready_future)
         * - Shared value storage (i.e.: shared futures created with
         * make_ready_future)
         * - Inline operation state (i.e.: static or deferred futures - address
         * can't change)
         * - Shared operation state (i.e.: eager and shared futures)
         *
         * Empty state: in other libraries, futures usually use a pointer to the
         * shared operation state. Because the operation state is not always
         * shared in this library, we need a variant type that represents the
         * empty states.
         *
         * Value storage: when the future already has a value, there's no point
         * in creating a unique or shared operation state to wrap this value.
         * The future can already store the value. This happens in two
         * situations: when we make_ready_future or when we move a future that's
         * ready. This removes overhead from the future while still allowing it
         * to store ready values, which are useful in recursive asynchronous
         * algorithms.
         *
         * Operation state storage: there are two kinds of operation state.
         * We always try to store this operation state inline. This is usually
         * with operation states that are not going to be moved. There are two
         * ways to indicate the operation state is not going to be moved after
         * execution starts:
         * (i) deferred futures on which we plan to call `wait`, or
         * (ii) explicitly defining this property in the future traits
         *
         * If we call `wait_for`/`wait_until` on a deferred future, the inline
         * operation state needs to be converted into a shared state because
         * the future might still be moved after we waited and before the
         * execution is over.
         *
         * The states can be converted from one type to the other as needed
         * by the parent future object. For instance, when a future is moved,
         * the previous future will receive an empty state while the new future
         * might receive the previous state or direct value storage, when this
         * is done.
         *
         * These deferred states are also considered non-copyable operation
         * states that are converted into shared state if they ever need to be
         * copied. This implies we only recur to a shared operation state when
         * we really need to. This allows us to avoid dynamic memory allocations
         * in all other cases.
         *
         * @note The documentation of this class is for exposition only. The
         * future state should never be accessed directly.
         *
         * @tparam R State main type
         * @tparam OpState Underlying operation state type
         *
         */
        template <class R, class OpState>
        class variant_state {
            FUTURES_STATIC_ASSERT(is_operation_state_v<OpState>);
            FUTURES_STATIC_ASSERT(std::is_move_constructible<OpState>::value);

            // The variant type used by the state
            // We use the never valueless variant2 type here
            using variant_type = boost::variant2::variant<
                boost::empty_init_t,
                operation_state_storage<R>,
                std::shared_ptr<operation_state_storage<R>>,
                OpState,
                std::shared_ptr<OpState>>;

            // Variant type for the states
            variant_type s_;

        public:
            using empty_type = boost::empty_init_t;
            using static_storage_type = operation_state_storage<R>;
            using shared_storage_type = std::shared_ptr<static_storage_type>;
            using static_operation_state_type = OpState;
            using shared_operation_state_type = std::shared_ptr<
                static_operation_state_type>;
        public:
            /*
             * Constructors
             */

            variant_state() = default;

            variant_state(variant_state const& other) {
                copy_impl(other);
            }

            variant_state(variant_state& other) {
                other.share_if_static();
                copy_impl(other);
            }

            variant_state(variant_state&& other) noexcept
                : s_(std::move(other.s_)) {}

            FUTURES_TEMPLATE(class T)
            (requires(mp_contains<variant_type, std::decay_t<T>>::
                          value)) explicit variant_state(T&& other)
                : s_(std::forward<T>(other)) {}

            FUTURES_TEMPLATE(class T, class... Args)
            (requires(
                mp_contains<variant_type, std::decay_t<T>>::
                    value)) explicit variant_state(in_place_type_t<T>, Args&&... args)
                : s_(
                    boost::variant2::in_place_type_t<T>{},
                    std::forward<Args>(args)...) {}

            variant_state&
            operator=(variant_state const& other) {
                copy_impl(other);
                return *this;
            }

            // Move Assignment
            /*
             * This operation leaves `other` in an empty state.
             *
             * @param other Other state
             */
            variant_state&
            operator=(variant_state<R, OpState>&& other) noexcept {
                s_ = std::move(other.s_);
                other.s_.template emplace<empty_type>();
                return *this;
            }

            /*
             * @}
             */

            /*
             * @name Accessors
             *
             * Variant-like functions
             *
             * @{
             */

            // Returns the index of the alternative held by the variant
            FUTURES_NODISCARD constexpr std::size_t
            index() const {
                return s_.index();
            }

            // Check if current variant value is of specified type
            /*
             * @note This function only participates in overload resolution of
             * `T` is one of the valid types of state
             *
             * @tparam T State type
             */
            FUTURES_TEMPLATE(class T)
            (requires(mp_contains<variant_type, std::decay_t<T>>::value))
                FUTURES_NODISCARD constexpr bool holds() const {
                return boost::variant2::holds_alternative<T>(s_);
            }

            // Check if variant value is empty value
            FUTURES_NODISCARD constexpr bool
            is_empty() const {
                return boost::variant2::holds_alternative<empty_type>(s_);
            }

            // Check if variant value is direct storage
            FUTURES_NODISCARD constexpr bool
            is_static_storage() const {
                return boost::variant2::holds_alternative<static_storage_type>(
                    s_);
            }

            // Check if variant value is shared direct storage
            FUTURES_NODISCARD constexpr bool
            is_shared_storage() const {
                return boost::variant2::holds_alternative<shared_storage_type>(
                    s_);
            }

            // Check if variant value is operation state
            FUTURES_NODISCARD constexpr bool
            is_static_operation_state() const {
                return boost::variant2::holds_alternative<
                    static_operation_state_type>(s_);
            }

            // Check if variant value is shared state
            FUTURES_NODISCARD bool
            is_shared_state() const {
                return boost::variant2::holds_alternative<
                    shared_operation_state_type>(s_);
            }

            // Get variant value as specified type
            FUTURES_TEMPLATE(class T)
            (requires(mp_contains<variant_type, std::decay_t<T>>::value))
                T& get_as() {
                return boost::variant2::get<T>(s_);
            }

            // Get constant variant value as specified type
            FUTURES_TEMPLATE(class T)
            (requires(
                mp_contains<variant_type, std::decay_t<T>>::value)) constexpr T
                const& get_as() const {
                return boost::variant2::get<T>(s_);
            }

            // Get variant value as empty value
            /*
             * This function accesses the variant value as a reference to
             * the empty value. This function will throw if the state
             * is not currently representing a empty value.
             * @return
             */
            empty_type&
            as_empty() {
                return get_as<empty_type>();
            }

            // @copydoc as_empty()
            FUTURES_NODISCARD empty_type const&
            as_empty() const {
                return get_as<empty_type>();
            }

            // Get variant value as storage
            /*
             * This function accesses the variant value as a reference to
             * the as storage. This function will throw if the state
             * is not currently representing a as storage.
             */
            static_storage_type&
            as_static_storage() {
                return get_as<static_storage_type>();
            }

            // @copydoc as_static_storage()
            FUTURES_NODISCARD static_storage_type const&
            as_static_storage() const {
                return get_as<static_storage_type>();
            }

            // Get variant value as shared storage
            /*
             * This function accesses the variant value as a reference to
             * the shared storage. This function will throw if the state
             * is not currently representing a shared storage.
             */
            shared_storage_type&
            as_shared_storage() {
                return get_as<shared_storage_type>();
            }

            // @copydoc as_shared_storage()
            FUTURES_NODISCARD shared_storage_type const&
            as_shared_storage() const {
                return get_as<shared_storage_type>();
            }

            // Get variant value as operation state
            /*
             * This function accesses the variant value as a reference to
             * the operation state. This function will throw if the state
             * is not currently representing a operation state.
             */
            OpState&
            as_static_operation_state() {
                return get_as<OpState>();
            }

            // @copydoc as_operation_state()
            FUTURES_NODISCARD OpState const&
            as_static_operation_state() const {
                return get_as<OpState>();
            }

            // Get variant value as shared state
            /*
             * This function accesses the variant value as a reference to
             * the shared state. This function will throw if the state
             * is not currently representing a shared state.
             */
            shared_operation_state_type&
            as_shared_state() {
                return get_as<shared_operation_state_type>();
            }

            // @copydoc as_shared_state()
            FUTURES_NODISCARD shared_operation_state_type const&
            as_shared_state() const {
                return get_as<shared_operation_state_type>();
            }

            // Constructs empty value in the variant in place
            /*
             * This function sets the operation state to empty value. The
             * previous value in the variant state is discarded.
             * @tparam Args
             * @param args
             */
            template <class... Args>
            void
            emplace_empty(Args&&... args) {
                s_.template emplace<empty_type>(std::forward<Args>(args)...);
            }

            // Constructs direct storage in the variant in place
            /*
             * This function sets the operation state to direct storage. The
             * previous value in the variant state is discarded.
             */
            template <class... Args>
            void
            emplace_storage(Args&&... args) {
                s_.template emplace<static_storage_type>(
                    std::forward<Args>(args)...);
            }

            // Constructs shared storage in the variant in place
            /*
             * This function sets the operation state to shared storage. The
             * previous value in the variant state is discarded.
             */
            template <class... Args>
            void
            emplace_shared_storage(Args&&... args) {
                s_.template emplace<shared_storage_type>(
                    std::forward<Args>(args)...);
            }

            // Constructs operation state in the variant in place
            /*
             * This function sets the operation state to operation state. The
             * previous value in the variant state is discarded.
             */
            template <class... Args>
            void
            emplace_operation_state(Args&&... args) {
                s_.template emplace<static_operation_state_type>(
                    std::forward<Args>(args)...);
            }

            // Constructs shared state in the variant in place
            /*
             * This function sets the operation state to shared state. The
             * previous value in the variant state is discarded.
             */
            template <class... Args>
            void
            emplace_shared_state(Args&&... args) {
                s_.template emplace<shared_operation_state_type>(
                    std::forward<Args>(args)...);
            }

            /*
             * @}
             */

            /*
             * @name Operation state functions
             *
             * Operation state-like functions. These functions are redirected
             * to the underlying operation state depending on the variant type
             * of state this object stores.
             *
             * @{
             */

            // Get the value of the operation state
            /*
             * This function waits for the operation state to become ready and
             * returns its value. It will forward the get function to proper
             * state type to get the type.
             *
             * @par Pre-conditions
             *
             * This function cannot be called on when the variant state
             * `is_empty()`.
             *
             * This function returns `R&` unless this is a operation state to
             * `void`, in which case `std::add_lvalue_reference_t<R>` is also
             * `void`.
             *
             * @return Reference to the state value of type R
             */
            ///
            std::add_lvalue_reference_t<
                typename static_operation_state_type::value_type>
            get() {
                if (is_shared_state()) {
                    return as_shared_state()->get();
                } else if (is_static_operation_state()) {
                    // emulating a const pointer to a shared state
                    return const_cast<static_operation_state_type&>(
                               as_static_operation_state())
                        .get();
                } else if (is_static_storage()) {
                    return as_static_storage().get();
                } else if (is_shared_storage()) {
                    return as_shared_storage()->get();
                }
                throw_exception(
                    std::invalid_argument{ "Operation state is invalid" });
            }

            // Get the operation state when it's as an exception
            /*
             * This function will forward `get_exception_ptr` function to proper
             * state type. If the variant state is empty, the function returns
             * nullptr.
             */
            std::exception_ptr
            get_exception_ptr() {
                if (is_shared_state()) {
                    return as_shared_state()->get_exception_ptr();
                } else if (is_static_operation_state()) {
                    return as_static_operation_state().get_exception_ptr();
                }
                return nullptr;
            }

            // Check if the current underlying operation state is valid
            FUTURES_NODISCARD bool
            valid() const {
                if (is_shared_state()) {
                    return as_shared_state().get() != nullptr;
                } else if (is_shared_storage()) {
                    return as_shared_storage().get() != nullptr;
                }
                return is_static_operation_state() || is_static_storage();
            }

            // Wait for operation state to become ready
            /*
             *  This function uses the condition variable waiters to wait for
             *  this operation state to be marked as ready. It will forward the
             *  wait function to proper underlying state type.
             */
            void
            wait() const {
                if (is_shared_state()) {
                    as_shared_state()->wait();
                } else if (is_static_operation_state()) {
                    // as far as the user is concerned, this is a const pointer
                    // to mutable shared state
                    const_cast<static_operation_state_type&>(
                        as_static_operation_state())
                        .wait();
                }
            }

            // Wait for the operation state to become ready
            /*
             *  This function uses the condition variable waiters to wait for
             *  this operation state to be marked as ready for a specified
             *  duration. It will forward the wait function to proper underlying
             *  state type.
             */
            template <class Rep, class Period>
            future_status
            wait_for(std::chrono::duration<Rep, Period> const& timeout_duration)
                const {
                if (is_static_operation_state()) {
                    // const_cast because as far as the user is concerned,
                    // we've been pretending inline operation states are
                    // potentially shared pointers all along
                    const_cast<variant_state*>(this)->share_if_static();
                }
                if (is_shared_state()) {
                    return as_shared_state()->wait_for(timeout_duration);
                }
                // empty or direct storage
                return future_status::ready;
            }

            // Forward wait_until function to proper state type

            // Wait for the operation state to become ready
            /*
             *  This function uses the condition variable waiters
             *  to wait for this operation state to be marked as ready until a
             *  specified time point. It will forward the wait function to
             * proper underlying state type.
             */
            template <class Clock, class Duration>
            future_status
            wait_until(std::chrono::time_point<Clock, Duration> const&
                           timeout_time) const {
                // Ensure an inline state type becomes shared because we cannot
                // guarantee what happens to the address after this operation
                // times out
                if (is_static_operation_state()) {
                    // const_cast because as far as the user is concerned,
                    // we've been pretending inline operation states are
                    // potentially shared pointers all along
                    const_cast<variant_state*>(this)->share_if_static();
                }
                if (is_shared_state()) {
                    return as_shared_state()->wait_until(timeout_time);
                }
                // empty or direct storage
                return future_status::ready;
            }

            // Check if operation state is ready
            /*
             * Forward is_ready function to proper state type
             */
            FUTURES_NODISCARD bool
            is_ready() const {
                if (is_shared_state()) {
                    return as_shared_state()->is_ready();
                } else if (is_static_operation_state()) {
                    return as_static_operation_state().is_ready();
                }
                return !is_empty();
            }

            // Get continuations_source from underlying operation state type
            typename static_operation_state_type::continuations_type&
            get_continuations_source() {
                if (is_shared_state()) {
                    return as_shared_state()->get_continuations_source();
                } else if (is_static_operation_state()) {
                    // emulates a const ptr to a mutable shared state
                    return const_cast<static_operation_state_type&>(
                               as_static_operation_state())
                        .get_continuations_source();
                }
                throw_exception(std::logic_error{ "Future non-continuable" });
            }

            // Include an external condition variable in the list of waiters
            typename static_operation_state_type::notify_when_ready_handle
            notify_when_ready(std::condition_variable_any& cv) {
                if (is_shared_state()) {
                    return as_shared_state()->notify_when_ready(cv);
                } else if (is_static_operation_state()) {
                    return as_static_operation_state().notify_when_ready(cv);
                }
                // Notify and return null handle
                cv.notify_all();
                return {};
            }

            // Remove condition variable from list of external waiters
            void
            unnotify_when_ready(
                typename static_operation_state_type::notify_when_ready_handle
                    h) {
                if (is_shared_state()) {
                    return as_shared_state()->unnotify_when_ready(h);
                } else if (is_static_operation_state()) {
                    return as_static_operation_state().unnotify_when_ready(h);
                }
                throw_exception(std::logic_error{ "Invalid type id" });
            }

            // Get stop_source from underlying operation state type
            FUTURES_NODISCARD stop_source
            get_stop_source() const noexcept {
                if (is_shared_state()) {
                    return as_shared_state()->get_stop_source();
                } else if (is_static_operation_state()) {
                    return as_static_operation_state().get_stop_source();
                } else if (is_static_storage() || is_shared_storage()) {
                    throw_exception(
                        std::logic_error{ "Cannot stop a ready future" });
                }
                throw_exception(std::logic_error{ "Invalid state" });
            }

            // Get stop_source from underlying operation state type
            FUTURES_NODISCARD const typename static_operation_state_type::
                executor_type&
                get_executor() const noexcept {
                if (is_shared_state()) {
                    return as_shared_state()->get_executor();
                } else if (is_static_operation_state()) {
                    return as_static_operation_state().get_executor();
                } else if (is_static_storage() || is_shared_storage()) {
                    throw_exception(std::logic_error{
                        "No associated executor to direct storage" });
                }
                throw_exception(std::logic_error{
                    "No associated executor to empty state" });
            }

            // Get a reference to the mutex in the operation state
            std::mutex&
            waiters_mutex() {
                if (is_shared_state()) {
                    return as_shared_state()->waiters_mutex();
                } else if (is_static_operation_state()) {
                    return as_static_operation_state().waiters_mutex();
                } else if (is_static_storage() || is_shared_storage()) {
                    throw_exception(std::logic_error{
                        "No associated executor to direct storage" });
                }
                throw_exception(std::logic_error{
                    "No associated executor to empty state" });
            }

            // Get number of futures pointing to the same operation state
            long
            use_count() const noexcept {
                if (is_shared_state()) {
                    return as_shared_state().use_count();
                } else if (is_shared_storage()) {
                    return as_shared_storage().use_count();
                }
                return !is_empty();
            }

            // Make sure the future object is shared
            void
            share_if_static() {
                if (is_static_storage()) {
                    emplace_shared_storage(
                        std::make_shared<static_storage_type>(
                            std::move(as_static_storage())));
                } else if (is_static_operation_state()) {
                    emplace_shared_state(
                        std::make_shared<static_operation_state_type>(
                            std::move(as_static_operation_state())));
                }
            }

            /*
             * @}
             */

        private:
            /*
             * Mutable/const implementations
             */

            /*
             * Helper Functions
             */
            // Copy the value from the other state, adapting as needed
            void
            copy_impl(variant_state const& other) {
                if (other.is_shared_state()) {
                    emplace_shared_state(other.as_shared_state());
                } else if (other.is_shared_storage()) {
                    emplace_shared_storage(other.as_shared_storage());
                } else if (other.is_empty()) {
                    emplace_empty(other.as_empty());
                }
                // Throw in use cases where the underlying basic_future is not
                // allowed to copy
                else if (is_static_storage() || is_static_operation_state())
                {
                    throw_exception(
                        std::logic_error{ "Inline states cannot be copied" });
                }
            }

            /*
             * @}
             */
        };
    } // namespace detail
} // namespace futures


#endif // FUTURES_DETAIL_VARIANT_STATE_HPP
