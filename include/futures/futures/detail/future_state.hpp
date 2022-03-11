//
// Copyright (c) 2022 alandefreitas (alandefreitas@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//

#ifndef FUTURES_FUTURES_DETAIL_FUTURE_STATE_HPP
#define FUTURES_FUTURES_DETAIL_FUTURE_STATE_HPP

#include <futures/detail/utility/aligned_storage_for.hpp>
#include <futures/detail/utility/empty_base.hpp>
#include <futures/futures/detail/operation_state.hpp>
#include <futures/futures/detail/operation_state_storage.hpp>
#include <futures/futures/detail/shared_state.hpp>

namespace futures::detail {
    /** @addtogroup futures Futures
     *  @{
     */
    /** @addtogroup future-traits Future Traits
     *  @{
     */

    /// Disambiguation tags that can be passed to the constructors future_state
    template <class T>
    struct in_place_type_t
    {
        explicit in_place_type_t() = default;
    };
    template <class T>
    inline constexpr in_place_type_t<T> in_place_type{};

    /// A variant operation state used in instances of basic_future
    /**
     * This class models the operation state in the various formats in might
     * be found in a future:
     *
     * - Empty state (i.e. default constructed and moved-from futures)
     * - Direct value storage (futures created with make_ready_future)
     * - Shared value storage (shared futures created with make_ready_future)
     * - Inline operation state (deferred futures - address can't change)
     * - Shared operation state (eager and shared futures)
     *
     * The non-copyable operation states are converted into shared state if
     * they ever need to be copied. This implies we only recur to a shared
     * operation state when we really need to. This allows us to avoid dynamic
     * memory allocations in all other cases.
     *
     * @tparam R State main type
     * @tparam OpState Underlying operation state type
     *
     */
    template <class R, class OpState>
    class future_state
    {
    private:
        /**
         * @name Private types
         * @{
         */

        static_assert(is_operation_state_v<OpState>);

        using empty_t = empty_value_type;
        using operation_storage_t = operation_state_storage<R>;
        using shared_storage_t = std::shared_ptr<operation_storage_t>;
        using operation_state_t = OpState;
        using shared_state_t = std::shared_ptr<operation_state_t>;

        template <class T>
        using is_future_state_type = std::disjunction<
            std::is_same<T, empty_t>,
            std::is_same<T, operation_storage_t>,
            std::is_same<T, shared_storage_t>,
            std::is_same<T, operation_state_t>,
            std::is_same<T, shared_state_t>>;

        template <class T>
        static constexpr bool is_future_state_type_v = is_future_state_type<
            T>::value;

        using aligned_storage_t = aligned_storage_for<
            empty_t,
            operation_storage_t,
            shared_storage_t,
            operation_state_t,
            shared_state_t>;

        /**
         * @}
         */
    public:
        /**
         * @name Public types
         * @{
         */

        /// Type ids for a future state
        enum class type_id : uint8_t
        {
            /// The future state is empty
            empty,
            /// The future state holds direct value storage
            direct_storage,
            /// The future state holds shared direct value storage
            shared_storage,
            /// The future state holds an inline operation state
            inline_state,
            /// The future state holds a shared operation state
            shared_state,
        };

        /**
         * @}
         */
    public:
        /**
         * @name Constructors
         * @{
         */

        /// Destructor
        ~future_state() {
            destroy_impl();
        };

        /// Constructor
        future_state() = default;

        /// Copy Constructor
        future_state(const future_state<R, operation_state_t>& other) {
            copy_impl(other);
        }

        /// Copy Constructor
        future_state(future_state<R, operation_state_t>& other) {
            other.share();
            copy_impl(other);
        }

        /// Move Constructor
        future_state(future_state<R, operation_state_t>&& other) noexcept {
            move_impl(std::move(other));
        }

        /// Converting Constructor
        template <
            class T
#ifndef FUTURES_DOXYGEN
            ,
            std::enable_if_t<is_future_state_type_v<std::decay_t<T>>, int> = 0
#endif
            >
        explicit future_state(T&& other) {
            using value_type = std::decay_t<T>;
            emplace<value_type>(std::forward<T>(other));
        }

        /// Emplace Constructor
        template <
            class T,
            class... Args
#ifndef FUTURES_DOXYGEN
            ,
            std::enable_if_t<is_future_state_type_v<std::decay_t<T>>, int> = 0
#endif
            >
        explicit future_state(in_place_type_t<T>, Args&&... args) {
            using value_type = std::decay_t<T>;
            emplace<value_type>(std::forward<Args>(args)...);
        }

        /// Copy Assignment
        future_state&
        operator=(const future_state<R, operation_state_t>& other) {
            copy_impl(other);
            return *this;
        }

        /// Move Assignment
        future_state&
        operator=(future_state<R, operation_state_t>&& other) noexcept {
            move_impl(std::move(other));
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

        /// Returns enumeration value of the type currently held by the variant
        [[nodiscard]] constexpr type_id
        type() const {
            return type_id_;
        }

        /// Returns the index of the alternative held by the variant
        [[nodiscard]] constexpr std::size_t
        index() const {
            return static_cast<std::size_t>(type());
        }

        /// Check if current variant value is of specified type
        template <
            class T
#ifndef FUTURES_DOXYGEN
            ,
            std::enable_if_t<is_future_state_type_v<std::decay_t<T>>, int> = 0
#endif
            >
        [[nodiscard]] bool
        holds() const {
            using value_type = std::decay_t<T>;
            if constexpr (is_future_state_type_v<value_type>) {
                return type_id_ == type_id_for<value_type>();
            }
            return false;
        }

        /// Check if variant value is empty value
        [[nodiscard]] bool
        holds_empty() const {
            return holds<empty_t>();
        }

        /// Check if variant value is direct storage
        [[nodiscard]] bool
        holds_storage() const {
            return holds<operation_storage_t>();
        }

        /// Check if variant value is shared direct storage
        [[nodiscard]] bool
        holds_shared_storage() const {
            return holds<shared_storage_t>();
        }

        /// Check if variant value is operation state
        [[nodiscard]] bool
        holds_operation_state() const {
            return holds<operation_state_t>();
        }

        /// Check if variant value is shared state
        [[nodiscard]] bool
        holds_shared_state() const {
            return holds<shared_state_t>();
        }

        /// Get variant value as specified type
        template <
            class T
#ifndef FUTURES_DOXYGEN
            ,
            std::enable_if_t<is_future_state_type_v<std::decay_t<T>>, int> = 0
#endif
            >
        T&
        get_as() {
            using value_type = std::decay_t<T>;
            if constexpr (is_future_state_type_v<value_type>) {
                return *reinterpret_cast<value_type*>(data());
            }
            detail::throw_exception<std::bad_cast>();
        }

        /// Get constant variant value as specified type
        template <
            class T
#ifndef FUTURES_DOXYGEN
            ,
            std::enable_if_t<is_future_state_type_v<std::decay_t<T>>, int> = 0
#endif
            >
        const T&
        get_as() const {
            using value_type = std::decay_t<T>;
            using return_type = std::add_const_t<value_type>;
            if constexpr (is_future_state_type_v<value_type>) {
                return *reinterpret_cast<return_type*>(data());
            }
            detail::throw_exception<std::bad_cast>();
        }

        /// Get variant value as empty value
        empty_t&
        get_as_empty() {
            return get_as<empty_t>();
        }

        /// Get constant variant value as empty value
        [[nodiscard]] const empty_t&
        get_as_empty() const {
            return get_as<empty_t>();
        }

        /// Get variant value as storage
        operation_storage_t&
        get_as_storage() {
            return get_as<operation_storage_t>();
        }

        /// Get constant variant value as storage
        [[nodiscard]] const operation_storage_t&
        get_as_storage() const {
            return get_as<operation_storage_t>();
        }

        /// Get variant value as shared storage
        shared_storage_t&
        get_as_shared_storage() {
            return get_as<shared_storage_t>();
        }

        /// Get constant variant value as shared storage
        [[nodiscard]] const shared_storage_t&
        get_as_shared_storage() const {
            return get_as<shared_storage_t>();
        }

        /// Get variant value as operation state
        operation_state_t&
        get_as_operation_state() {
            return get_as<operation_state_t>();
        }

        /// Get constant variant value as operation state
        [[nodiscard]] const operation_state_t&
        get_as_operation_state() const {
            return get_as<operation_state_t>();
        }

        /// Get variant value as shared state
        shared_state_t&
        get_as_shared_state() {
            return get_as<shared_state_t>();
        }

        /// Get constant variant value as shared state
        [[nodiscard]] const shared_state_t&
        get_as_shared_state() const {
            return get_as<shared_state_t>();
        }

        /// Constructs a value in the variant, in place
        template <
            class T,
            class... Args
#ifndef FUTURES_DOXYGEN
            ,
            std::enable_if_t<is_future_state_type_v<std::decay_t<T>>, int> = 0
#endif
            >
        void
        emplace(Args&&... args) {
            destroy_impl();
            new (data()) T(std::forward<Args>(args)...);
            type_id_ = type_id_for<T>();
        }

        /// Constructs empty value in the variant, in place
        template <class... Args>
        void
        emplace_empty(Args&&... args) {
            return emplace<empty_t>(std::forward<Args>(args)...);
        }

        /// Constructs direct storage in the variant, in place
        template <class... Args>
        void
        emplace_storage(Args&&... args) {
            return emplace<operation_storage_t>(std::forward<Args>(args)...);
        }

        /// Constructs shared storage in the variant, in place
        template <class... Args>
        void
        emplace_shared_storage(Args&&... args) {
            return emplace<shared_storage_t>(std::forward<Args>(args)...);
        }

        /// Constructs operation state in the variant, in place
        template <class... Args>
        void
        emplace_operation_state(Args&&... args) {
            return emplace<operation_state_t>(std::forward<Args>(args)...);
        }

        /// Constructs shared state in the variant, in place
        template <class... Args>
        void
        emplace_shared_state(Args&&... args) {
            return emplace<shared_state_t>(std::forward<Args>(args)...);
        }

        /**
         * @}
         */

        /**
         * @name Operation state functions
         *
         * Operation state-like functions. These function are redirected
         * to the operation state depending on the type of state this
         * object stores.
         *
         * @{
         */

        /// Forward get function to proper state type
        decltype(auto)
        get() {
            switch (type_id_) {
            case type_id::shared_state:
                return get_as_shared_state()->get();
            case type_id::inline_state:
                return get_as_operation_state().get();
            case type_id::direct_storage:
                return get_as_storage().get();
            case type_id::shared_storage:
                return get_as_shared_storage()->get();
            case type_id::empty:
                detail::throw_exception<std::logic_error>(
                    "Operation state is invalid");
            }
            detail::throw_exception<std::logic_error>("Invalid type id");
        }

        /// Forward get_exception_ptr function to proper state type
        std::exception_ptr
        get_exception_ptr() {
            switch (type_id_) {
            case type_id::shared_state:
                return get_as_shared_state()->get_exception_ptr();
            case type_id::inline_state:
                return get_as_operation_state().get_exception_ptr();
            case type_id::direct_storage:
            case type_id::shared_storage:
            case type_id::empty:
                return nullptr;
            }
            detail::throw_exception<std::logic_error>("Invalid type id");
        }

        [[nodiscard]] bool
        valid() const {
            switch (type_id_) {
            case type_id::shared_state:
                return get_as_shared_state().get() != nullptr;
            case type_id::inline_state:
                return true;
            case type_id::direct_storage:
                return true;
            case type_id::shared_storage:
                return get_as_shared_storage().get() != nullptr;
            case type_id::empty:
                return false;
            }
            detail::throw_exception<std::logic_error>("Invalid type id");
        }

    private:
        template <bool is_const>
        static void
        wait_impl(
            std::conditional_t<is_const, const future_state, future_state>& s) {
            switch (s.type_id_) {
            case type_id::shared_state:
                s.get_as_shared_state()->wait();
                return;
            case type_id::inline_state:
                s.get_as_operation_state().wait();
                return;
            case type_id::direct_storage:
            case type_id::shared_storage:
            case type_id::empty:
                return;
            }
            detail::throw_exception<std::logic_error>("Invalid type id");
        }
    public:
        /// Forward wait function to proper state type
        void
        wait() const {
            wait_impl<true>(*this);
        }

        /// Forward wait function to proper state type
        void
        wait() {
            wait_impl<false>(*this);
        }

    private:
        template <bool is_const, class Rep, class Period>
        static std::future_status
        wait_for_impl(
            std::conditional_t<is_const, const future_state, future_state>& s,
            const std::chrono::duration<Rep, Period>& timeout_duration) {
            // Ensure the state type is shared
            if constexpr (!is_const) {
                if (s.type_id_ == type_id::inline_state) {
                    s.share();
                }
            }
            switch (s.type_id_) {
            case type_id::shared_state:
                return s.get_as_shared_state()->wait_for(timeout_duration);
            case type_id::direct_storage:
            case type_id::shared_storage:
            case type_id::empty:
                return std::future_status::ready;
            case type_id::inline_state:
                detail::throw_exception<std::logic_error>(
                    "Cannot wait for deferred state with timeout");
            }
            detail::throw_exception<std::logic_error>("Invalid type id");
        }
    public:
        /// Forward wait_for function to proper state type
        template <class Rep, class Period>
        std::future_status
        wait_for(
            const std::chrono::duration<Rep, Period>& timeout_duration) const {
            return wait_for_impl<true>(*this, timeout_duration);
        }

        template <class Rep, class Period>
        std::future_status
        wait_for(const std::chrono::duration<Rep, Period>& timeout_duration) {
            return wait_for_impl<false>(*this, timeout_duration);
        }

    private:
        template <bool is_const, class Clock, class Duration>
        static std::future_status
        wait_until_impl(
            std::conditional_t<is_const, const future_state, future_state>& s,
            const std::chrono::time_point<Clock, Duration>& timeout_time) {
            // Ensure the state type is shared
            if constexpr (is_const) {
                if (s.type_id_ == type_id::inline_state) {
                    s.share();
                }
            }
            switch (s.type_id_) {
            case type_id::shared_state:
                return s.get_as_shared_state()->wait_until(timeout_time);
            case type_id::direct_storage:
            case type_id::shared_storage:
            case type_id::empty:
                return std::future_status::ready;
            case type_id::inline_state:
                detail::throw_exception<std::logic_error>(
                    "Cannot wait for deferred state with timeout");
            }
            detail::throw_exception<std::logic_error>("Invalid type id");
        }
    public:
        /// Forward wait_until function to proper state type
        template <class Clock, class Duration>
        std::future_status
        wait_until(const std::chrono::time_point<Clock, Duration>& timeout_time)
            const {
            return wait_until_impl<true>(*this, timeout_time);
        }

        /// Forward wait_until function to proper state type
        template <class Clock, class Duration>
        std::future_status
        wait_until(
            const std::chrono::time_point<Clock, Duration>& timeout_time) {
            return wait_until_impl<false>(*this, timeout_time);
        }

        /// Forward is_ready function to proper state type
        [[nodiscard]] bool
        is_ready() const {
            switch (type_id_) {
            case type_id::shared_state:
                return get_as_shared_state()->is_ready();
            case type_id::inline_state:
                return get_as_operation_state().is_ready();
            case type_id::direct_storage:
            case type_id::shared_storage:
                return true;
            case type_id::empty:
                return false;
            }
            detail::throw_exception<std::logic_error>("Invalid type id");
        }

        /// Forward get_continuations_source function to proper state type
        auto
        get_continuations_source() {
            switch (type_id_) {
            case type_id::shared_state:
                return get_as_shared_state()->get_continuations_source();
            case type_id::inline_state:
                return get_as_operation_state().get_continuations_source();
            case type_id::direct_storage:
            case type_id::shared_storage:
            case type_id::empty:
                detail::throw_exception<std::logic_error>(
                    "Future non-continuable");
            }
            detail::throw_exception<std::logic_error>("Invalid type id");
        }

        typename operation_state_t::notify_when_ready_handle
        notify_when_ready(std::condition_variable_any& cv) {
            switch (type_id_) {
            case type_id::shared_state:
                return get_as_shared_state()->notify_when_ready(cv);
            case type_id::inline_state:
                return get_as_operation_state().notify_when_ready(cv);
            case type_id::direct_storage:
            case type_id::shared_storage:
            case type_id::empty:
                cv.notify_all();
                return typename operation_state_t::notify_when_ready_handle{};
            }
            detail::throw_exception<std::logic_error>("Invalid type id");
        }

        void
        unnotify_when_ready(
            typename operation_state_t::notify_when_ready_handle h) {
            switch (type_id_) {
            case type_id::shared_state:
                return get_as_shared_state()->unnotify_when_ready(h);
            case type_id::inline_state:
                return get_as_operation_state().unnotify_when_ready(h);
            case type_id::direct_storage:
            case type_id::shared_storage:
            case type_id::empty:
                (void) h;
            }
            detail::throw_exception<std::logic_error>("Invalid type id");
        }

        [[nodiscard]] stop_source
        get_stop_source() const noexcept {
            switch (type_id_) {
            case type_id::shared_state:
                return get_as_shared_state()->get_stop_source();
            case type_id::inline_state:
                return get_as_operation_state().get_stop_source();
            case type_id::direct_storage:
            case type_id::shared_storage:
                detail::throw_exception<std::logic_error>(
                    "Cannot stop a ready future");
            case type_id::empty:
                detail::throw_exception<std::logic_error>("Invalid state");
            }
            detail::throw_exception<std::logic_error>("Invalid type id");
        }

        [[nodiscard]] const typename operation_state_options_t<
            OpState>::executor_t&
        get_executor() const noexcept {
            switch (type_id_) {
            case type_id::shared_state:
                return get_as_shared_state()->get_executor();
            case type_id::inline_state:
                return get_as_operation_state().get_executor();
            case type_id::direct_storage:
            case type_id::shared_storage:
                detail::throw_exception<std::logic_error>(
                    "No associated executor to direct storage");
            case type_id::empty:
                detail::throw_exception<std::logic_error>(
                    "No associated executor to empty state");
            }
            detail::throw_exception<std::logic_error>("Invalid type id");
        }

        std::mutex&
        waiters_mutex() {
            switch (type_id_) {
            case type_id::shared_state:
                return get_as_shared_state()->waiters_mutex();
            case type_id::inline_state:
                return get_as_operation_state().waiters_mutex();
            case type_id::direct_storage:
            case type_id::shared_storage:
                detail::throw_exception<std::logic_error>(
                    "No associated executor to direct storage");
            case type_id::empty:
                detail::throw_exception<std::logic_error>(
                    "No associated executor to empty state");
            }
            detail::throw_exception<std::logic_error>("Invalid type id");
        }

        long
        use_count() const noexcept {
            switch (type_id_) {
            case type_id::shared_state:
                return get_as_shared_state().use_count();
            case type_id::shared_storage:
                return get_as_shared_storage().use_count();
            case type_id::inline_state:
            case type_id::direct_storage:
                return 1;
            case type_id::empty:
                return 0;
            }
            detail::throw_exception<std::logic_error>("Invalid type id");
        }

        /// Make sure the future object is shared
        void
        share() {
            if (holds_storage()) {
                emplace_shared_storage(std::make_shared<operation_storage_t>(
                    std::move(get_as_storage())));
            }
            if (holds_operation_state()) {
                emplace_shared_state(std::make_shared<operation_state_t>(
                    std::move(get_as_operation_state())));
            }
        }

        /**
         * @}
         */

    private:
        /**
         * @name Helper Functions
         * @{
         */
        constexpr byte*
        data() {
            return data_.data();
        }

        constexpr const byte*
        data() const {
            return data_.data();
        }

        void
        destroy_impl() {
            switch (type_id_) {
            case type_id::empty:
                break;
            case type_id::direct_storage:
                get_as_storage().~operation_storage_t();
                break;
            case type_id::shared_storage:
                get_as_shared_storage().~shared_storage_t();
                break;
            case type_id::inline_state:
                get_as_operation_state().~operation_state_t();
                break;
            case type_id::shared_state:
                get_as_shared_state().~shared_state_t();
                break;
            }
            type_id_ = type_id::empty;
        }

        void
        copy_impl(const future_state& other) {
            destroy_impl();
            switch (other.type_id_) {
            case type_id::empty:
                emplace_empty();
                break;
            case type_id::shared_storage:
                emplace_shared_storage(other.get_as_shared_storage());
                break;
            case type_id::shared_state:
                emplace_shared_state(other.get_as_shared_state());
                break;
            case type_id::direct_storage:
            case type_id::inline_state:
            default:
                detail::throw_exception<std::logic_error>(
                    "No copy constructor");
            }
        }

        void
        move_impl(future_state&& other) {
            destroy_impl();
            switch (other.type_id_) {
            case type_id::empty:
                emplace_empty();
                break;
            case type_id::direct_storage:
                emplace_storage(std::move(other.get_as_storage()));
                break;
            case type_id::shared_storage:
                emplace_shared_storage(
                    std::move(other.get_as_shared_storage()));
                break;
            case type_id::inline_state:
                emplace_operation_state(
                    std::move(other.get_as_operation_state()));
                break;
            case type_id::shared_state:
                emplace_shared_state(std::move(other.get_as_shared_state()));
                break;
            }
            // destroy the object moved from and set its type back to empty
            other.destroy_impl();
        }

        template <class T>
        static constexpr type_id
        type_id_for() {
            if constexpr (std::is_same_v<T, empty_t>) {
                return type_id::empty;
            }
            if constexpr (std::is_same_v<T, operation_storage_t>) {
                return type_id::direct_storage;
            }
            if constexpr (std::is_same_v<T, shared_storage_t>) {
                return type_id::shared_storage;
            }
            if constexpr (std::is_same_v<T, operation_state_t>) {
                return type_id::inline_state;
            }
            if constexpr (std::is_same_v<T, shared_state_t>) {
                return type_id::shared_state;
            }
            detail::throw_exception<std::invalid_argument>("Invalid type T");
        }

        /**
         * @}
         */

        /**
         * @name Members
         * @{
         */

        aligned_storage_t data_;
        type_id type_id_{ type_id::empty };

        /**
         * @}
         */
    };

    /** @} */
    /** @} */
} // namespace futures::detail


#endif // FUTURES_FUTURES_DETAIL_FUTURE_STATE_HPP
