//
// Copyright (c) 2022 alandefreitas (alandefreitas@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//

#ifndef FUTURES_DETAIL_VARIANT_STATE_HPP
#define FUTURES_DETAIL_VARIANT_STATE_HPP

#include <futures/config.hpp>
#include <futures/detail/operation_state.hpp>
#include <futures/detail/operation_state_storage.hpp>
#include <futures/detail/shared_state.hpp>
#include <futures/detail/utility/byte.hpp>
#include <futures/detail/deps/boost/core/empty_value.hpp>
#include <futures/detail/deps/boost/mp11/algorithm.hpp>
#include <futures/detail/deps/boost/variant2/variant.hpp>

namespace futures::detail {
    /** @addtogroup futures Futures
     *  @{
     */

    /// Disambiguation tags that can be passed to the constructors future_state
    /**
     * @par Example
     * @code
     * // Construct future state with my_type as initial state
     * future_state<my_type> s(in_place_type<my_type>, my_type_arguments, ...);
     * @endcode
     *
     * @tparam T Type the tag represents
     */
    template <class T>
    struct in_place_type_t {
        explicit in_place_type_t() = default;
    };

    template <class T>
    inline constexpr in_place_type_t<T> in_place_type{};

    /*
     * Exposition only
     */

    /// The variant operation state used in instances of @ref basic_future
    /**
     * This class models an operation state in the various formats it might
     * be found in a future:
     *
     * - Empty state (i.e. default constructed and moved-from futures)
     * - Direct value storage (futures created with make_ready_future)
     * - Shared value storage (shared futures created with make_ready_future)
     * - Inline operation state (deferred futures - address can't change)
     * - Shared operation state (eager and shared futures)
     *
     * Empty state: in other libraries, futures usually use a pointer to the
     * shared operation state. Because the operation state is not always
     * shared in this library, we need a variant type that represents the
     * empty states.
     *
     * Value storage: when the future already has a value, there's no point
     * in creating an unique or shared operation state to wrap this value.
     * The future can already store the value. This happens in two situations:
     * when we make_ready_future or when we move a future that's ready.
     * This removes overhead from the future while still allowing it
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
     * the future might still be moved after we waited and before the execution
     * is over.
     *
     * The states can be converted from one type to the other as needed
     * by the parent future object. For instance, when a future is moved, the
     * previous future will receive an empty state while the new future might
     * receive the previous state or direct value storage, when this is done.
     *
     * These deferred states are also considered non-copyable operation states
     * that are converted into shared state if they ever need to be copied.
     * This implies we only recur to a shared operation state when we really
     * need to. This allows us to avoid dynamic memory allocations in all
     * other cases.
     *
     * @note The documentation of this class is for exposition only. The future
     * state should never be accessed directly.
     *
     * @tparam R State main type
     * @tparam OpState Underlying operation state type
     *
     */
    template <class R, class OpState>
    class variant_state {
        static_assert(is_operation_state_v<OpState>);

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
        /**
         * @name Public types
         * @{
         */

        using empty_type = boost::empty_init_t;
        using operation_storage_type = operation_state_storage<R>;
        using shared_storage_type = std::shared_ptr<operation_storage_type>;
        using operation_state_type = OpState;
        using shared_state_type = std::shared_ptr<operation_state_type>;

        /**
         * @}
         */
    public:
        /**
         * @name Constructors
         * @{
         */

        /// Constructor
        variant_state() = default;

        /// Copy Constructor
        /**
         * This function will adapt the type the other operation state holds
         * to the type possible for this operation state.
         *
         * @param other Other state
         */
        variant_state(const variant_state& other) {
            copy_impl(other);
        }

        /// Constructor
        /**
         * This function will adapt the type the other operation state holds
         * to the type possible for this operation state.
         *
         * @param other Other state
         */
        variant_state(variant_state& other) {
            other.share();
            copy_impl(other);
        }

        /// Constructor
        /**
         * The other type will be left in an empty state
         *
         * @param other Other state
         */
        variant_state(variant_state&& other) noexcept
            : s_(std::move(other.s_)) {
            other.s_.template emplace<empty_type>();
        }

        /// Constructor
        /**
         * Construct the variant with a value of type `T`, where `T` is
         * one value state for this operation.
         *
         * This function only participates in overload resolution
         * if `T`
         *
         * @tparam T
         */
        template <
            class T
#ifndef FUTURES_DOXYGEN
            // clang-format off
            , std::enable_if_t<mp_contains<variant_type, std::decay_t<T>>::value, int> = 0
        // clang-format on
#endif
            >
        explicit variant_state(T&& other) : s_(std::forward<T>(other)) {
        }

        /// Constructor
        /**
         * Construct for a specific variant type.
         *
         * @tparam T
         * @tparam Args
         * @param args
         */
        template <
            class T,
            class... Args
#ifndef FUTURES_DOXYGEN
            // clang-format off
            , std::enable_if_t<mp_contains<variant_type, std::decay_t<T>>::value, int> = 0
        // clang-format on
#endif
            >
        explicit variant_state(in_place_type_t<T>, Args&&... args)
            : s_(boost::variant2::in_place_type<T>, std::forward<Args>(args)...) {
        }

        /// Copy Assignment
        variant_state&
        operator=(const variant_state& other) {
            copy_impl(other);
            return *this;
        }

        /// Move Assignment
        /**
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

        /**
         * @}
         */

        /**
         * @name Accessors
         *
         * Variant-like functions
         *
         * @{
         */

        /// Returns the index of the alternative held by the variant
        [[nodiscard]] constexpr std::size_t
        index() const {
            return s_.index();
        }

        /// Check if current variant value is of specified type
        /**
         * @note This function only participates in overload resolution of
         * `T` is one of the valid types of state
         *
         * @tparam T State type
         */
        template <
            class T
#ifndef FUTURES_DOXYGEN
            // clang-format off
            , std::enable_if_t<mp_contains<variant_type, std::decay_t<T>>::value, int> = 0
        // clang-format on
#endif
            >
        [[nodiscard]] constexpr bool
        holds() const {
            return boost::variant2::holds_alternative<T>(s_);
        }

        /// Check if variant value is empty value
        [[nodiscard]] constexpr bool
        is_empty() const {
            return boost::variant2::holds_alternative<empty_type>(s_);
        }

        /// Check if variant value is direct storage
        [[nodiscard]] constexpr bool
        is_storage() const {
            return boost::variant2::holds_alternative<operation_storage_type>(
                s_);
        }

        /// Check if variant value is shared direct storage
        [[nodiscard]] constexpr bool
        is_shared_storage() const {
            return boost::variant2::holds_alternative<shared_storage_type>(s_);
        }

        /// Check if variant value is operation state
        [[nodiscard]] constexpr bool
        is_operation_state() const {
            return boost::variant2::holds_alternative<operation_state_type>(s_);
        }

        /// Check if variant value is shared state
        [[nodiscard]] bool
        is_shared_state() const {
            return boost::variant2::holds_alternative<shared_state_type>(s_);
        }

        /// Get variant value as specified type
        template <
            class T
#ifndef FUTURES_DOXYGEN
            // clang-format off
            , std::enable_if_t<mp_contains<variant_type, std::decay_t<T>>::value, int> = 0
        // clang-format on
#endif
            >
        T&
        get_as() {
            return boost::variant2::get<T>(s_);
        }

        /// Get constant variant value as specified type
        template <
            class T
#ifndef FUTURES_DOXYGEN
            // clang-format off
            , std::enable_if_t<mp_contains<variant_type, std::decay_t<T>>::value, int> = 0
        // clang-format on
#endif
            >
        constexpr const T&
        get_as() const {
            return boost::variant2::get<T>(s_);
        }

        /// Get variant value as empty value
        /**
         * This function accesses the variant value as a reference to
         * the empty value. This function will throw if the state
         * is not currently representing a empty value.
         * @return
         */
        empty_type&
        get_as_empty() {
            return get_as<empty_type>();
        }

        /// @copydoc get_as_empty()
        [[nodiscard]] const empty_type&
        get_as_empty() const {
            return get_as<empty_type>();
        }

        /// Get variant value as storage
        /**
         * This function accesses the variant value as a reference to
         * the as storage. This function will throw if the state
         * is not currently representing a as storage.
         */
        operation_storage_type&
        get_as_storage() {
            return get_as<operation_storage_type>();
        }

        /// @copydoc get_as_storage()
        [[nodiscard]] const operation_storage_type&
        get_as_storage() const {
            return get_as<operation_storage_type>();
        }

        /// Get variant value as shared storage
        /**
         * This function accesses the variant value as a reference to
         * the shared storage. This function will throw if the state
         * is not currently representing a shared storage.
         */
        shared_storage_type&
        get_as_shared_storage() {
            return get_as<shared_storage_type>();
        }

        /// @copydoc get_as_shared_storage()
        [[nodiscard]] const shared_storage_type&
        get_as_shared_storage() const {
            return get_as<shared_storage_type>();
        }

        /// Get variant value as operation state
        /**
         * This function accesses the variant value as a reference to
         * the operation state. This function will throw if the state
         * is not currently representing a operation state.
         */
        OpState&
        get_as_operation_state() {
            return get_as<OpState>();
        }

        /// @copydoc get_as_operation_state()
        [[nodiscard]] const OpState&
        get_as_operation_state() const {
            return get_as<OpState>();
        }

        /// Get variant value as shared state
        /**
         * This function accesses the variant value as a reference to
         * the shared state. This function will throw if the state
         * is not currently representing a shared state.
         */
        shared_state_type&
        get_as_shared_state() {
            return get_as<shared_state_type>();
        }

        /// @copydoc get_as_shared_state()
        [[nodiscard]] const shared_state_type&
        get_as_shared_state() const {
            return get_as<shared_state_type>();
        }

        /// Constructs empty value in the variant in place
        /**
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

        /// Constructs direct storage in the variant in place
        /**
         * This function sets the operation state to direct storage. The
         * previous value in the variant state is discarded.
         */
        template <class... Args>
        void
        emplace_storage(Args&&... args) {
            s_.template emplace<operation_storage_type>(
                std::forward<Args>(args)...);
        }

        /// Constructs shared storage in the variant in place
        /**
         * This function sets the operation state to shared storage. The
         * previous value in the variant state is discarded.
         */
        template <class... Args>
        void
        emplace_shared_storage(Args&&... args) {
            s_.template emplace<shared_storage_type>(
                std::forward<Args>(args)...);
        }

        /// Constructs operation state in the variant in place
        /**
         * This function sets the operation state to operation state. The
         * previous value in the variant state is discarded.
         */
        template <class... Args>
        void
        emplace_operation_state(Args&&... args) {
            s_.template emplace<operation_state_type>(
                std::forward<Args>(args)...);
        }

        /// Constructs shared state in the variant in place
        /**
         * This function sets the operation state to shared state. The
         * previous value in the variant state is discarded.
         */
        template <class... Args>
        void
        emplace_shared_state(Args&&... args) {
            s_.template emplace<shared_state_type>(std::forward<Args>(args)...);
        }

        /**
         * @}
         */

        /**
         * @name Operation state functions
         *
         * Operation state-like functions. These functions are redirected
         * to the underlying operation state depending on the variant type
         * of state this object stores.
         *
         * @{
         */

        /// Get the value of the operation state
        /**
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
        std::add_lvalue_reference_t<typename operation_state_type::value_type>
        get() {
            if (is_shared_state())
                return get_as_shared_state()->get();
            else if (is_operation_state())
                return get_as_operation_state().get();
            else if (is_storage())
                return get_as_storage().get();
            else if (is_shared_storage())
                return get_as_shared_storage()->get();
            detail::throw_exception<std::invalid_argument>(
                "Operation state is invalid");
        }

        /// Get the operation state when it's as an exception
        /**
         * This function will forward `get_exception_ptr` function to proper
         * state type. If the variant state is empty, the function returns
         * nullptr.
         */
        std::exception_ptr
        get_exception_ptr() {
            if (is_shared_state())
                return get_as_shared_state()->get_exception_ptr();
            else if (is_operation_state())
                return get_as_operation_state().get_exception_ptr();
            return nullptr;
        }

        /// Check if the current underlying operation state is valid
        [[nodiscard]] bool
        valid() const {
            if (is_shared_state())
                return get_as_shared_state().get() != nullptr;
            else if (is_shared_storage())
                return get_as_shared_storage().get() != nullptr;
            return is_operation_state() || is_storage();
        }

        /// Wait for operation state to become ready
        /**
         *  This function uses the condition variable waiters to wait for
         *  this operation state to be marked as ready. It will forward the
         *  wait function to proper underlying state type.
         */
        void
        wait() const {
            wait_impl<true>(*this);
        }

        /// @copydoc wait()
        void
        wait() {
            wait_impl<false>(*this);
        }

        /// Wait for the operation state to become ready
        /**
         *  This function uses the condition variable waiters to wait for
         *  this operation state to be marked as ready for a specified
         *  duration. It will forward the wait function to proper underlying
         *  state type.
         */
        template <class Rep, class Period>
        std::future_status
        wait_for(
            const std::chrono::duration<Rep, Period>& timeout_duration) const {
            return wait_for_impl<true>(*this, timeout_duration);
        }

        /// @copydoc wait_for()
        template <class Rep, class Period>
        std::future_status
        wait_for(const std::chrono::duration<Rep, Period>& timeout_duration) {
            return wait_for_impl<false>(*this, timeout_duration);
        }

        /// Forward wait_until function to proper state type

        /// Wait for the operation state to become ready
        /**
         *  This function uses the condition variable waiters
         *  to wait for this operation state to be marked as ready until a
         *  specified time point. It will forward the wait function to proper
         *  underlying state type.
         */
        template <class Clock, class Duration>
        std::future_status
        wait_until(const std::chrono::time_point<Clock, Duration>& timeout_time)
            const {
            return wait_until_impl<true>(*this, timeout_time);
        }

        /// @copydoc wait_until()
        template <class Clock, class Duration>
        std::future_status
        wait_until(
            const std::chrono::time_point<Clock, Duration>& timeout_time) {
            return wait_until_impl<false>(*this, timeout_time);
        }

        /// Check if operation state is ready
        /**
         * Forward is_ready function to proper state type
         */
        [[nodiscard]] bool
        is_ready() const {
            if (is_shared_state())
                return get_as_shared_state()->is_ready();
            else if (is_operation_state())
                return get_as_operation_state().is_ready();
            return !is_empty();
        }

        /// Get continuations_source from underlying operation state type
        typename operation_state_type::continuations_type&
        get_continuations_source() {
            if (is_shared_state())
                return get_as_shared_state()->get_continuations_source();
            else if (is_operation_state())
                return get_as_operation_state().get_continuations_source();
            detail::throw_exception<std::logic_error>("Future non-continuable");
        }

        /// Include an external condition variable in the list of waiters
        typename operation_state_type::notify_when_ready_handle
        notify_when_ready(std::condition_variable_any& cv) {
            if (is_shared_state())
                return get_as_shared_state()->notify_when_ready(cv);
            else if (is_operation_state())
                return get_as_operation_state().notify_when_ready(cv);
            // Notify and return null handle
            cv.notify_all();
            return {};
        }

        /// Remove condition variable from list of external waiters
        void
        unnotify_when_ready(
            typename operation_state_type::notify_when_ready_handle h) {
            if (is_shared_state())
                return get_as_shared_state()->unnotify_when_ready(h);
            else if (is_operation_state())
                return get_as_operation_state().unnotify_when_ready(h);
            detail::throw_exception<std::logic_error>("Invalid type id");
        }

        /// Get stop_source from underlying operation state type
        [[nodiscard]] stop_source
        get_stop_source() const noexcept {
            if (is_shared_state())
                return get_as_shared_state()->get_stop_source();
            else if (is_operation_state())
                return get_as_operation_state().get_stop_source();
            else if (is_storage() || is_shared_storage())
                detail::throw_exception<std::logic_error>(
                    "Cannot stop a ready future");
            detail::throw_exception<std::logic_error>("Invalid state");
        }

        /// Get stop_source from underlying operation state type
        [[nodiscard]] const typename operation_state_type::executor_type&
        get_executor() const noexcept {
            if (is_shared_state())
                return get_as_shared_state()->get_executor();
            else if (is_operation_state())
                return get_as_operation_state().get_executor();
            else if (is_storage() || is_shared_storage())
                detail::throw_exception<std::logic_error>(
                    "No associated executor to direct storage");
            detail::throw_exception<std::logic_error>(
                "No associated executor to empty state");
        }

        /// Get a reference to the mutex in the operation state
        std::mutex&
        waiters_mutex() {
            if (is_shared_state())
                return get_as_shared_state()->waiters_mutex();
            else if (is_operation_state())
                return get_as_operation_state().waiters_mutex();
            else if (is_storage() || is_shared_storage())
                detail::throw_exception<std::logic_error>(
                    "No associated executor to direct storage");
            detail::throw_exception<std::logic_error>(
                "No associated executor to empty state");
        }

        /// Get number of futures pointing to the same operation state
        long
        use_count() const noexcept {
            if (is_shared_state())
                return get_as_shared_state().use_count();
            else if (is_shared_storage())
                return get_as_shared_storage().use_count();
            return !is_empty();
        }

        /// Make sure the future object is shared
        void
        share() {
            if (is_storage()) {
                emplace_shared_storage(std::make_shared<operation_storage_type>(
                    std::move(get_as_storage())));
            } else if (is_operation_state()) {
                emplace_shared_state(std::make_shared<operation_state_type>(
                    std::move(get_as_operation_state())));
            }
        }

        /**
         * @}
         */

    private:
        /*
         * Mutable/const implementations
         */
        template <bool B, class T>
        using add_const_if = std::conditional_t<B, std::add_const_t<T>, T>;

        template <bool is_const>
        static void
        wait_impl(add_const_if<is_const, variant_state>& s) {
            if (s.is_shared_state())
                s.get_as_shared_state()->wait();
            else if (s.is_operation_state())
                s.get_as_operation_state().wait();
            return;
        }

        // Share the operation state if it's inlined
        // This ensures an inline state type becomes shared because we cannot
        // guarantee what happens to the address after this operation
        // times out. However, if the state is const and the state is inline,
        // there's nothing we can do here, and we need to throw an exception
        // because this operation is never safe.
        constexpr static void
        share_inline(variant_state& s) {
            if (s.is_operation_state()) {
                s.share();
            }
        }

        constexpr static void
        share_inline(std::add_const<variant_state>&) {}

        template <bool is_const, class Rep, class Period>
        static std::future_status
        wait_for_impl(
            add_const_if<is_const, variant_state>& s,
            const std::chrono::duration<Rep, Period>& timeout_duration) {
            share_inline(s);
            if (s.is_shared_state())
                return s.get_as_shared_state()->wait_for(timeout_duration);
            else if (s.is_operation_state())
                detail::throw_exception<std::invalid_argument>(
                    "Cannot wait for a const deferred state with a timeout");
            return std::future_status::ready;
        }

        template <bool is_const, class Clock, class Duration>
        static std::future_status
        wait_until_impl(
            add_const_if<is_const, variant_state>& s,
            const std::chrono::time_point<Clock, Duration>& timeout_time) {
            // Ensure an inline state type becomes shared because we cannot
            // guarantee what happens to the address after this operation
            // times out
            share_inline(s);
            if (s.is_shared_state())
                return s.get_as_shared_state()->wait_until(timeout_time);
            else if (s.is_operation_state())
                detail::throw_exception<std::invalid_argument>(
                    "Cannot wait for a const deferred state with timeout");
            return std::future_status::ready;
        }

        /*
         * Helper Functions
         */
        // Copy the value from the other state, adapting as needed
        void
        copy_impl(const variant_state& other) {
            if (other.is_shared_state())
                emplace_shared_state(other.get_as_shared_state());
            else if (other.is_shared_storage())
                emplace_shared_storage(other.get_as_shared_storage());
            else if (other.is_empty())
                emplace_empty(other.get_as_empty());
            // Throw in use cases where the future is not allowed to copy
            if (is_storage() || is_operation_state())
                detail::throw_exception<std::logic_error>(
                    "Inline states cannot be copied");
        }

        /**
         * @}
         */
    };

    /** @} */
} // namespace futures::detail


#endif // FUTURES_DETAIL_VARIANT_STATE_HPP
