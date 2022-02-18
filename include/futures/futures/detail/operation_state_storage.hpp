//
// Copyright (c) 2022 alandefreitas (alandefreitas@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//

#ifndef FUTURES_FUTURES_DETAIL_OPERATION_STATE_STORAGE_HPP
#define FUTURES_FUTURES_DETAIL_OPERATION_STATE_STORAGE_HPP

#include <futures/detail/utility/empty_base.hpp>
#include <type_traits>

namespace futures::detail {
    /// Determine the type we should use to store a shared state
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
    struct operation_state_storage
    {};

    template <typename R>
    struct operation_state_storage<R, std::enable_if_t<std::is_void_v<R>>>
    {
        template <class... Args>
        void
        set_value(Args&&...) {}

        void
        get() {}

        void destroy() {}
    };

    template <typename R>
    struct operation_state_storage<
        R,
        std::enable_if_t<
            // clang-format off
            !std::is_void_v<R> &&
            std::is_reference_v<R>
            // clang-format on
            >>
    {
        void
        set_value(R& value) {
            value_ = std::addressof(value);
        }

        R&
        get() {
            assert(value_);
            return *value_;
        }

        void destroy() {}

        std::decay_t<R>* value_{ nullptr };
    };

    template <typename R>
    struct operation_state_storage<
        R,
        std::enable_if_t<
            // clang-format off
            !std::is_void_v<R> &&
            !std::is_reference_v<R> &&
            std::is_trivial_v<R>
            // clang-format on
            >>
    {
        void
        set_value(const R& value) {
            value_ = value;
        }

        R&
        get() {
            return value_;
        }

        void destroy() {}

        R value_;
    };

    template <typename R>
    struct operation_state_storage<
        R,
        std::enable_if_t<
            // clang-format off
            !std::is_void_v<R> &&
            !std::is_reference_v<R> &&
            !std::is_trivial_v<R>
            // clang-format on
            >>
    {
        template <class... Args>
        void
        set_value(Args&&... args) {
            ::new (static_cast<void*>(std::addressof(value_)))
                R(std::forward<Args>(args)...);
        }

        R&
        get() {
            return *reinterpret_cast<R *>(std::addressof(value_));
        }

        void destroy() {
            reinterpret_cast<R *>(std::addressof(value_))->~R();
        }

        std::aligned_storage_t<sizeof(R), alignof(R)> value_{};
    };

} // namespace futures::detail

#endif // FUTURES_FUTURES_DETAIL_OPERATION_STATE_STORAGE_HPP
