//
// Copyright (c) 2023 alandefreitas (alandefreitas@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//

#ifndef FUTURES_DETAIL_UTILITY_MOVE_ONLY_FUNCTION_HPP
#define FUTURES_DETAIL_UTILITY_MOVE_ONLY_FUNCTION_HPP

#include <futures/config.hpp>
#include <futures/detail/utility/invoke.hpp>
#include <futures/detail/utility/sbo_ptr.hpp>

namespace futures {
    namespace detail {
        /*
         * This is somewhat similar to C++23 move_only_function, with a few
         * differences: custom SBO size and custom allocator.
         *
         * https://en.cppreference.com/w/cpp/utility/functional/move_only_function
         */

        /*
         * Callable interface
         */
        template <class Signature>
        struct move_only_function_interface;

#define FUTURES_NO_ARG /* msvc hack */
#define FUTURES_DECL_MOVE_ONLY_INTERFACE_FOR(expr)         \
    template <class R, class... Args>                      \
    struct move_only_function_interface<R(Args...) expr> { \
        virtual ~move_only_function_interface() = default; \
        virtual R                                          \
        operator()(Args...) expr                           \
            = 0;                                           \
    }
        FUTURES_DECL_MOVE_ONLY_INTERFACE_FOR(FUTURES_NO_ARG);
        FUTURES_DECL_MOVE_ONLY_INTERFACE_FOR(&);
        FUTURES_DECL_MOVE_ONLY_INTERFACE_FOR(&&);
        FUTURES_DECL_MOVE_ONLY_INTERFACE_FOR(const);
        FUTURES_DECL_MOVE_ONLY_INTERFACE_FOR(const &);
        FUTURES_DECL_MOVE_ONLY_INTERFACE_FOR(const &&);
#ifdef __cpp_noexcept_function_type
        FUTURES_DECL_MOVE_ONLY_INTERFACE_FOR(noexcept);
        FUTURES_DECL_MOVE_ONLY_INTERFACE_FOR(&noexcept);
        FUTURES_DECL_MOVE_ONLY_INTERFACE_FOR(&&noexcept);
        FUTURES_DECL_MOVE_ONLY_INTERFACE_FOR(const noexcept);
        FUTURES_DECL_MOVE_ONLY_INTERFACE_FOR(const &noexcept);
        FUTURES_DECL_MOVE_ONLY_INTERFACE_FOR(const &&noexcept);
#endif
#undef FUTURES_DECL_MOVE_ONLY_INTERFACE_FOR

        /*
         * Callable interface implementation
         */
        template <class F, class Signature>
        class move_only_function_interface_impl;

        template <typename T>
        constexpr typename std::remove_reference<T>::type &&
        maybe_move_impl(std::true_type, T &&t) noexcept {
            // return rvalue
            return std::move(t);
        }

        template <typename T>
        constexpr typename std::remove_reference<T>::type &
        maybe_move_impl(std::false_type, T &&t) noexcept {
            // return lvalue
            return static_cast<T &>(t);
        }

        template <typename T>
        constexpr std::add_const_t<T> &
        maybe_as_const_impl(std::true_type, T &t) noexcept {
            // return const
            return t;
        }

        template <typename T>
        constexpr T &
        maybe_as_const_impl(std::false_type, T &t) noexcept {
            // return mutable
            return t;
        }

#define FUTURES_DECL_MOVE_ONLY_IMPL_FOR(expr, do_move, do_const)               \
    template <class F, class R, class... Args>                                 \
    class move_only_function_interface_impl<F, R(Args...) expr>                \
        : public move_only_function_interface<R(Args...) expr> {               \
        F f_;                                                                  \
    public:                                                                    \
        virtual ~move_only_function_interface_impl() = default;                \
                                                                               \
        move_only_function_interface_impl(                                     \
            move_only_function_interface_impl &&) noexcept                     \
            = default;                                                         \
                                                                               \
        move_only_function_interface_impl(                                     \
            move_only_function_interface_impl const &)                         \
            = default;                                                         \
                                                                               \
        FUTURES_TEMPLATE(class U)                                              \
        (requires std::is_convertible<U, F>::value)                            \
            move_only_function_interface_impl(U &&u)                           \
            : f_(std::forward<U>(u)) {}                                        \
                                                                               \
        virtual R                                                              \
        operator()(Args... args) expr {                                        \
            auto &f = maybe_as_const_impl(bool_constant<do_const>{}, f_);      \
            return ::futures::detail::                                         \
                invoke(maybe_move_impl(bool_constant<do_move>{}, f), args...); \
        }                                                                      \
    }

        FUTURES_DECL_MOVE_ONLY_IMPL_FOR(FUTURES_NO_ARG, false, false);
        FUTURES_DECL_MOVE_ONLY_IMPL_FOR(&, false, false);
        FUTURES_DECL_MOVE_ONLY_IMPL_FOR(&&, true, false);
        FUTURES_DECL_MOVE_ONLY_IMPL_FOR(const, false, true);
        FUTURES_DECL_MOVE_ONLY_IMPL_FOR(const &, false, true);
        FUTURES_DECL_MOVE_ONLY_IMPL_FOR(const &&, true, true);
#ifdef __cpp_noexcept_function_type
        FUTURES_DECL_MOVE_ONLY_IMPL_FOR(noexcept, false, false);
        FUTURES_DECL_MOVE_ONLY_IMPL_FOR(&noexcept, false, false);
        FUTURES_DECL_MOVE_ONLY_IMPL_FOR(&&noexcept, true, false);
        FUTURES_DECL_MOVE_ONLY_IMPL_FOR(const noexcept, false, true);
        FUTURES_DECL_MOVE_ONLY_IMPL_FOR(const &noexcept, false, true);
        FUTURES_DECL_MOVE_ONLY_IMPL_FOR(const &&noexcept, true, true);
#endif
#undef FUTURES_DECL_MOVE_ONLY_IMPL_FOR

        /*
         * Base class with sbo_pointer and operator()
         */
        template <class Signature, class Allocator, std::size_t N>
        class move_only_function_base;

#define FUTURES_DECL_MOVE_ONLY_BASE_FOR(expr, do_move, do_const)              \
    template <class R, class... Args, class Allocator, std::size_t N>         \
    class move_only_function_base<R(Args...) expr, Allocator, N>              \
        : public move_only_function_interface<R(Args...) expr> {              \
    protected:                                                                \
        template <class F>                                                    \
        using impl_type                                                       \
            = move_only_function_interface_impl<F, R(Args...) expr>;          \
        using interface_type = move_only_function_interface<R(Args...) expr>; \
        using sbo_ptr_type = move_only_sbo_ptr<interface_type, Allocator, N>; \
        sbo_ptr_type impl_;                                                   \
        struct from_sbo_ptr_tag {};                                           \
    public:                                                                   \
        virtual ~move_only_function_base() = default;                         \
        using result_type = R;                                                \
        move_only_function_base() = default;                                  \
                                                                              \
        move_only_function_base(from_sbo_ptr_tag, sbo_ptr_type &&u)           \
            : impl_(std::move(u)){};                                          \
                                                                              \
        FUTURES_TEMPLATE(class U)                                             \
        (requires std::is_convertible<                                        \
            impl_type<std::decay_t<U>> *,                                     \
            interface_type *>::value) move_only_function_base(U &&u)          \
            : impl_(impl_type<std::decay_t<U>>(std::forward<U>(u))) {}        \
                                                                              \
        template <class T, class... CArgs>                                    \
        explicit move_only_function_base(in_place_type_t<T>, CArgs &&...args) \
            : impl_(                                                          \
                in_place_type_t<impl_type<std::decay_t<T>>>{},                \
                std::forward<CArgs>(args)...) {}                              \
                                                                              \
        virtual R                                                             \
        operator()(Args... args) expr {                                       \
            auto &f = maybe_as_const_impl(bool_constant<do_const>{}, impl_);  \
            return maybe_move_impl(bool_constant<do_move>{}, *f)              \
                .                                                             \
                operator()(args...);                                          \
        }                                                                     \
    }

        FUTURES_DECL_MOVE_ONLY_BASE_FOR(FUTURES_NO_ARG, false, false);
        FUTURES_DECL_MOVE_ONLY_BASE_FOR(&, false, false);
        FUTURES_DECL_MOVE_ONLY_BASE_FOR(&&, true, false);
        FUTURES_DECL_MOVE_ONLY_BASE_FOR(const, false, true);
        FUTURES_DECL_MOVE_ONLY_BASE_FOR(const &, false, true);
        FUTURES_DECL_MOVE_ONLY_BASE_FOR(const &&, true, true);
#ifdef __cpp_noexcept_function_type
        FUTURES_DECL_MOVE_ONLY_BASE_FOR(noexcept, false, false);
        FUTURES_DECL_MOVE_ONLY_BASE_FOR(&noexcept, false, false);
        FUTURES_DECL_MOVE_ONLY_BASE_FOR(&&noexcept, true, false);
        FUTURES_DECL_MOVE_ONLY_BASE_FOR(const noexcept, false, true);
        FUTURES_DECL_MOVE_ONLY_BASE_FOR(const &noexcept, false, true);
        FUTURES_DECL_MOVE_ONLY_BASE_FOR(const &&noexcept, true, true);
#endif
#undef FUTURES_DECL_MOVE_ONLY_IMPL_FOR
#undef FUTURES_NO_ARG

        /*
         * Main class
         */

        template <
            class Signature,
            class Allocator = std::allocator<int *>,
            std::size_t N = sizeof(int *) * 4>
        class move_only_function
            : protected move_only_function_base<Signature, Allocator, N> {
            using Base = move_only_function_base<Signature, Allocator, N>;
        public:
            using result_type = typename Base::result_type;

            // Empty function
            move_only_function() noexcept = default;

            // Empty function
            move_only_function(std::nullptr_t) noexcept {}

            // Cannot copy construct
            move_only_function(move_only_function const &) = delete;

            // Move constructor
            move_only_function(move_only_function &&other) noexcept
                : Base(
                    typename Base::from_sbo_ptr_tag{},
                    std::move(other.impl_)) {}

            // Construct for callable f
            template <class F>
            move_only_function(F &&f) : Base(std::forward<F>(f)) {}

            // Construct callable in place
            template <class T, class... CArgs>
            explicit move_only_function(in_place_type_t<T>, CArgs &&...args)
                : Base(in_place_type_t<T>{}, std::forward<CArgs>(args)...) {}

            // Destructor
            ~move_only_function() = default;

            // Move assignment
            move_only_function &
            operator=(move_only_function &&other) {
                Base::impl_ = std::move(other.Base::impl_);
                return *this;
            }

            // Cannot copy assign
            move_only_function &
            operator=(move_only_function const &)
                = delete;

            // Reset
            move_only_function &
            operator=(std::nullptr_t) noexcept {
                Base::impl_.reset();
                return *this;
            }

            // Emplace
            template <class F>
            move_only_function &
            operator=(F &&f) {
                Base::impl_.template emplace<
                    typename Base::template impl_type<std::decay_t<F>>>(
                    std::forward<F>(f));
                return *this;
            }

            // Swap
            void
            swap(move_only_function &other) noexcept {
                std::swap(Base::impl_, other.Base::impl_);
            }

            // Checks whether *this stores a callable target, i.e. is not empty.
            explicit operator bool() const noexcept {
                return Base::impl_.operator bool();
            }

            // Compares a std::move_only_function with std::nullptr_t
            friend bool
            operator==(move_only_function const &f, std::nullptr_t) noexcept {
                return f;
            }

            // Invokes the stored callable target with the parameters args
            // Calling operator comes from base
            using Base::operator();
        };

    } // namespace detail
} // namespace futures

#endif // FUTURES_DETAIL_UTILITY_MOVE_ONLY_FUNCTION_HPP
