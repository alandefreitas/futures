/// \file
// Range v3 library
//
//  Copyright Eric Niebler 2013-present
//  Copyright Casey Carter 2016
//
//  Use, modification and distribution is subject to the
//  Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
//
// Project home: https://github.com/ericniebler/range-v3
//
#ifndef FUTURES_RANGES_FUNCTIONAL_INVOKE_HPP
#define FUTURES_RANGES_FUNCTIONAL_INVOKE_HPP

#include <functional>
#include <type_traits>

#include <futures/algorithm/detail/traits/range/meta/meta.h>

#include <futures/algorithm/detail/traits/range/concepts/concepts.h>

#include <futures/algorithm/detail/traits/range/range_fwd.h>

#include <futures/algorithm/detail/traits/range/utility/static_const.h>

#include <futures/algorithm/detail/traits/range/detail/prologue.h>

RANGES_DIAGNOSTIC_PUSH
RANGES_DIAGNOSTIC_IGNORE_CXX17_COMPAT
RANGES_DIAGNOSTIC_IGNORE_DEPRECATED_DECLARATIONS

#ifndef RANGES_CONSTEXPR_INVOKE
#ifdef RANGES_WORKAROUND_CLANG_23135
#define RANGES_CONSTEXPR_INVOKE 0
#else
#define RANGES_CONSTEXPR_INVOKE 1
#endif
#endif

namespace futures::detail {
    /// \addtogroup group-functional
    /// @{

    /// \cond
    namespace ranges_detail {
        RANGES_DIAGNOSTIC_PUSH
        RANGES_DIAGNOSTIC_IGNORE_VOID_PTR_DEREFERENCE

        template <typename U> U &can_reference_(U &&);

        // clang-format off
        template<typename T>
        CPP_requires(dereferenceable_part_,
            requires(T && t) //
            (
                ranges_detail::can_reference_(*(T &&) t)
            ));
        template<typename T>
        CPP_concept dereferenceable_ = //
            CPP_requires_ref(ranges_detail::dereferenceable_part_, T);
        // clang-format on

        RANGES_DIAGNOSTIC_POP

        template <typename T>
        RANGES_INLINE_VAR constexpr bool is_reference_wrapper_v =
            futures::detail::meta::is<T, reference_wrapper>::value || futures::detail::meta::is<T, std::reference_wrapper>::value;
    } // namespace ranges_detail
    /// \endcond

    template <typename T>
    RANGES_INLINE_VAR constexpr bool is_reference_wrapper_v = ranges_detail::is_reference_wrapper_v<ranges_detail::decay_t<T>>;

    template <typename T> using is_reference_wrapper = futures::detail::meta::bool_<is_reference_wrapper_v<T>>;

    /// \cond
    template <typename T>
    using is_reference_wrapper_t
        RANGES_DEPRECATED("is_reference_wrapper_t is deprecated.") = futures::detail::meta::_t<is_reference_wrapper<T>>;
    /// \endcond

    struct invoke_fn {
      private:
        template(typename, typename T1)(
            /// \pre
            requires ranges_detail::dereferenceable_<T1>) static constexpr decltype(auto)
            coerce(T1 &&t1, long) noexcept(noexcept(*static_cast<T1 &&>(t1))) {
            return *static_cast<T1 &&>(t1);
        }

        template(typename T, typename T1)(
            /// \pre
            requires derived_from<ranges_detail::decay_t<T1>, T>) static constexpr T1 &&coerce(T1 &&t1, int) noexcept {
            return static_cast<T1 &&>(t1);
        }

        template(typename, typename T1)(
            /// \pre
            requires ranges_detail::is_reference_wrapper_v<ranges_detail::decay_t<T1>>) static constexpr decltype(auto)
            coerce(T1 &&t1, int) noexcept {
            return static_cast<T1 &&>(t1).get();
        }

      public:
        template <typename F, typename T, typename T1, typename... Args>
        constexpr auto operator()(F T::*f, T1 &&t1, Args &&...args) const
            noexcept(noexcept((invoke_fn::coerce<T>((T1 &&) t1, 0).*f)((Args &&) args...)))
                -> decltype((invoke_fn::coerce<T>((T1 &&) t1, 0).*f)((Args &&) args...)) {
            return (invoke_fn::coerce<T>((T1 &&) t1, 0).*f)((Args &&) args...);
        }

        template <typename D, typename T, typename T1>
        constexpr auto operator()(D T::*f, T1 &&t1) const noexcept(noexcept(invoke_fn::coerce<T>((T1 &&) t1, 0).*f))
            -> decltype(invoke_fn::coerce<T>((T1 &&) t1, 0).*f) {
            return invoke_fn::coerce<T>((T1 &&) t1, 0).*f;
        }

        template <typename F, typename... Args>
        CPP_PP_IIF(RANGES_CONSTEXPR_INVOKE)
        (CPP_PP_EXPAND, CPP_PP_EAT)(constexpr) auto operator()(F &&f, Args &&...args) const
            noexcept(noexcept(((F &&) f)((Args &&) args...))) -> decltype(((F &&) f)((Args &&) args...)) {
            return ((F &&) f)((Args &&) args...);
        }
    };

    RANGES_INLINE_VARIABLE(invoke_fn, invoke)

#ifdef RANGES_WORKAROUND_MSVC_701385
    /// \cond
    namespace ranges_detail {
        template <typename Void, typename Fun, typename... Args> struct _invoke_result_ {};

        template <typename Fun, typename... Args>
        struct _invoke_result_<futures::detail::meta::void_<decltype(invoke(std::declval<Fun>(), std::declval<Args>()...))>, Fun,
                               Args...> {
            using type = decltype(invoke(std::declval<Fun>(), std::declval<Args>()...));
        };
    } // namespace ranges_detail
    /// \endcond

    template <typename Fun, typename... Args> using invoke_result = ranges_detail::_invoke_result_<void, Fun, Args...>;

    template <typename Fun, typename... Args> using invoke_result_t = futures::detail::meta::_t<invoke_result<Fun, Args...>>;

#else  // RANGES_WORKAROUND_MSVC_701385
    template <typename Fun, typename... Args>
    using invoke_result_t = decltype(invoke(std::declval<Fun>(), std::declval<Args>()...));

    template <typename Fun, typename... Args> struct invoke_result : futures::detail::meta::defer<invoke_result_t, Fun, Args...> {};
#endif // RANGES_WORKAROUND_MSVC_701385

    /// \cond
    namespace ranges_detail {
        template <bool IsInvocable> struct is_nothrow_invocable_impl_ {
            template <typename Fn, typename... Args> static constexpr bool apply() noexcept { return false; }
        };
        template <> struct is_nothrow_invocable_impl_<true> {
            template <typename Fn, typename... Args> static constexpr bool apply() noexcept {
                return noexcept(invoke(std::declval<Fn>(), std::declval<Args>()...));
            }
        };
    } // namespace ranges_detail
    /// \endcond

    template <typename Fn, typename... Args>
    RANGES_INLINE_VAR constexpr bool is_invocable_v = futures::detail::meta::is_trait<invoke_result<Fn, Args...>>::value;

    template <typename Fn, typename... Args>
    RANGES_INLINE_VAR constexpr bool is_nothrow_invocable_v =
        ranges_detail::is_nothrow_invocable_impl_<is_invocable_v<Fn, Args...>>::template apply<Fn, Args...>();

    /// \cond
    template <typename Sig>
    struct RANGES_DEPRECATED("futures::detail::result_of is deprecated. "
                             "Please use futures::detail::invoke_result") result_of {};

    template <typename Fun, typename... Args>
    struct RANGES_DEPRECATED("futures::detail::result_of is deprecated. "
                             "Please use futures::detail::invoke_result") result_of<Fun(Args...)>
        : futures::detail::meta::defer<invoke_result_t, Fun, Args...> {};
    /// \endcond

    namespace cpp20 {
        using ::futures::detail::invoke;
        using ::futures::detail::invoke_result;
        using ::futures::detail::invoke_result_t;
        using ::futures::detail::is_invocable_v;
        using ::futures::detail::is_nothrow_invocable_v;
    } // namespace cpp20

    /// @}
} // namespace futures::detail

RANGES_DIAGNOSTIC_POP

#include <futures/algorithm/detail/traits/range/detail/epilogue.h>

#endif // FUTURES_RANGES_FUNCTIONAL_INVOKE_HPP
