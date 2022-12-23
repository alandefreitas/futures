//
// Copyright (c) 2022 alandefreitas (alandefreitas@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//

#ifndef FUTURES_DETAIL_OPERATION_STATE_STORAGE_HPP
#define FUTURES_DETAIL_OPERATION_STATE_STORAGE_HPP

#include <futures/throw.hpp>
#include <futures/detail/traits/std_type_traits.hpp>
#include <futures/detail/deps/boost/core/empty_value.hpp>
#include <futures/detail/deps/boost/optional/optional.hpp>
#include <type_traits>

namespace futures {
    namespace detail {
        // Determine the type we should use to store a shared state internally
        /*
         * We usually need uninitialized storage for a given type, since the
         * shared state needs to be in control of constructors and destructors.
         *
         * For trivial types, we can directly store the value.
         *
         * When the shared state is a reference, we store pointers internally.
         */
        template <typename R, class Enable = void>
        class operation_state_storage {};

        // Operation state storage for void
        /*
         * When the operation state value type is void, we don't need to store
         * anything. The function are simply ignored.
         *
         * @tparam R Operation state value type
         */
        template <typename R>
        class operation_state_storage<R, std::enable_if_t<is_void_v<R>>> {
        public:
            operation_state_storage() = default;

            template <class... Args>
            void
            set_value(Args&&...) {}

            void
            get() {}
        };

        // Operation state storage for references
        /*
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
            !is_void_v<R> &&
            is_reference_v<R>
                // clang-format on
                >> {
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

        // Operation state storage for trivial types
        /*
         * When the operation state value type is trivial, we store the value
         * directly. This means we can directly visualize the current value of
         * the operation state at any time, even when not explicitly
         * initialized.
         *
         * @tparam R Operation state value type
         */
        template <typename R>
        class operation_state_storage<
            R,
            std::enable_if_t<
                // clang-format off
            !is_void_v<R> &&
            !is_reference_v<R> &&
            is_trivial_v<R>
                // clang-format on
                >> {
        public:
            operation_state_storage() = default;

            explicit operation_state_storage(R const& value) {
                set_value(value);
            }

            explicit operation_state_storage(R&& value) {
                set_value(std::move(value));
            }

            void
            set_value(R const& value) {
                value_ = value;
            }

            R&
            get() {
                return value_;
            }

        private:
            R value_;
        };

        // Operation state storage for non-trivial types
        /*
         * When the operation state value type is not trivial, we store the
         * value with inline aligned store. This acts like a std::optional that
         * allows us to keep an uninitialized value while the promise has not
         * been set yet. However, whether the optional has a value is controlled
         * by the parent operation state.
         *
         * @tparam R Operation state value type
         */
        template <typename R>
        class operation_state_storage<
            R,
            std::enable_if_t<
                // clang-format off
            !is_void_v<R> &&
            !is_reference_v<R> &&
            !is_trivial_v<R>
                // clang-format on
                >> {
            boost::optional<R> data_{};

        public:
            operation_state_storage() = default;

            template <class... Args>
            explicit operation_state_storage(Args&&... args) {
                set_value(std::forward<Args>(args)...);
            }

            template <class... Args>
            void
            set_value(Args&&... args) {
                data_.emplace(std::forward<Args>(args)...);
            }

            R&
            get() {
                if (data_) {
                    return *data_;
                }
                throw_exception(promise_uninitialized{});
            }
        };

    } // namespace detail
} // namespace futures

#endif // FUTURES_DETAIL_OPERATION_STATE_STORAGE_HPP
