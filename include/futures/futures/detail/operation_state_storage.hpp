//
// Copyright (c) 2022 alandefreitas (alandefreitas@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//

#ifndef FUTURES_FUTURES_DETAIL_OPERATION_STATE_STORAGE_HPP
#define FUTURES_FUTURES_DETAIL_OPERATION_STATE_STORAGE_HPP

#include <futures/detail/utility/aligned_storage_for.hpp>
#include <futures/detail/utility/empty_base.hpp>
#include <type_traits>

namespace futures::detail {
    /// Determine the type we should use to store a shared state internally
    /**
     * We usually need uninitialized storage for a given type, since the
     * shared state needs to be in control of constructors and destructors.
     *
     * For trivial types, we can directly store the value.
     *
     * When the shared state is a reference, we store pointers internally.
     */
    template <typename R, class Enable = void>
    class operation_state_storage
    {};

    /// Operation state storage for void
    /**
     * When the operation state value type is void, we don't need to store
     * anything. The function are simply ignored.
     *
     * @tparam R Operation state value type
     */
    template <typename R>
    class operation_state_storage<R, std::enable_if_t<std::is_void_v<R>>>
    {
    public:
        operation_state_storage() = default;

        template <class... Args>
        void
        set_value(Args&&...) {}

        void
        get() {}
    };

    /// Operation state storage for references
    /**
     * When the operation state value type is a reference, we internally
     * store a pointer. Whenever we access the value, the pointer is
     * dereferenced into a reference again.
     *
     * @tparam R Operation state value type
     */
    template <typename R>
    class operation_state_storage<
        R,
        std::enable_if_t<
            // clang-format off
            !std::is_void_v<R> &&
            std::is_reference_v<R>
            // clang-format on
            >>
    {
    public:
        operation_state_storage() = default;

        explicit operation_state_storage(R& value) {
            set_value(value);
        }

        void
        set_value(R& value) {
            value_ = std::addressof(value);
        }

        R&
        get() {
            assert(value_);
            return *value_;
        }

    private:
        std::decay_t<R>* value_{ nullptr };
    };

    /// Operation state storage for trivial types
    /**
     * When the operation state value type is trivial, we store the value
     * directly. This means we can directly visualize the current value of
     * the operation state at any time, even when not explicitly initialized.
     *
     * @tparam R Operation state value type
     */
    template <typename R>
    class operation_state_storage<
        R,
        std::enable_if_t<
            // clang-format off
            !std::is_void_v<R> &&
            !std::is_reference_v<R> &&
            std::is_trivial_v<R>
            // clang-format on
            >>
    {
    public:
        operation_state_storage() = default;

        explicit operation_state_storage(const R& value) {
            set_value(value);
        }

        explicit operation_state_storage(R&& value) {
            set_value(std::move(value));
        }

        void
        set_value(const R& value) {
            value_ = value;
        }

        R&
        get() {
            return value_;
        }

    private:
        R value_;
    };

    /// Operation state storage for non-trivial types
    /**
     * When the operation state value type is not trivial, we store the value
     * with inline aligned store. This acts like a std::optional that allows us
     * to keep an uninitialized value while the promise has not been set yet.
     * However, whether the optional has a value is controlled by the parent
     * operation state.
     *
     * @tparam R Operation state value type
     */
    template <typename R>
    class operation_state_storage<
        R,
        std::enable_if_t<
            // clang-format off
            !std::is_void_v<R> &&
            !std::is_reference_v<R> &&
            !std::is_trivial_v<R>
            // clang-format on
            >>
    {
    public:
        ~operation_state_storage() {
            if (has_value_) {
                get().~R();
            }
        }

        operation_state_storage() = default;

        operation_state_storage(const operation_state_storage& other) {
            set_value(other.get());
        }

        operation_state_storage(operation_state_storage&& other) noexcept {
            set_value(std::move(other.get()));
        }

        template <class... Args>
        explicit operation_state_storage(Args&&... args) {
            set_value(std::forward<Args>(args)...);
        }

        operation_state_storage&
        operator=(const operation_state_storage& other) {
            set_value(other.get());
        }

        operation_state_storage&
        operator=(operation_state_storage&& other) noexcept {
            set_value(other.get());
        }

        template <class... Args>
        void
        set_value(Args&&... args) {
            if (has_value_) {
                destroy();
            }
            ::new (static_cast<void*>(value_.data()))
                R(std::forward<Args>(args)...);
            has_value_ = true;
        }

        R&
        get() {
            if (has_value_) {
                return *reinterpret_cast<R*>(value_.data());
            }
            detail::throw_exception<promise_uninitialized>();
        }

    private:
        void
        destroy() {
            if (has_value_) {
                get().~R();
                has_value_ = false;
            }
        }

        detail::aligned_storage_for<R> value_{};
        bool has_value_{ false };
    };

} // namespace futures::detail

#endif // FUTURES_FUTURES_DETAIL_OPERATION_STATE_STORAGE_HPP
