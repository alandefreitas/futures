/// \file
// Range v3 library
//
//  Copyright Eric Niebler 2013-present
//
//  Use, modification and distribution is subject to the
//  Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
//
// Project home: https://github.com/ericniebler/range-v3
//
#ifndef FUTURES_RANGES_FUNCTIONAL_REFERENCE_WRAPPER_HPP
#define FUTURES_RANGES_FUNCTIONAL_REFERENCE_WRAPPER_HPP

#include <type_traits>
#include <utility>

#include <futures/algorithm/detail/traits/range/meta/meta.h>

#include <futures/algorithm/detail/traits/range/concepts/concepts.h>

#include <futures/algorithm/detail/traits/range/functional/invoke.h>
#include <futures/algorithm/detail/traits/range/utility/addressof.h>
#include <futures/algorithm/detail/traits/range/utility/static_const.h>

#include <futures/algorithm/detail/traits/range/detail/prologue.h>

namespace futures::detail {
    /// \addtogroup group-functional
    /// @{

    /// \cond
    namespace ranges_detail {
        template <typename T> struct reference_wrapper_ {
            T *t_ = nullptr;
            constexpr reference_wrapper_() = default;
            constexpr reference_wrapper_(T &t) noexcept : t_(ranges_detail::addressof(t)) {}
            constexpr reference_wrapper_(T &&) = delete;
            constexpr T &get() const noexcept { return *t_; }
        };
        template <typename T> struct reference_wrapper_<T &> : reference_wrapper_<T> {
            using reference_wrapper_<T>::reference_wrapper_;
        };
        template <typename T> struct reference_wrapper_<T &&> {
            T *t_ = nullptr;
            constexpr reference_wrapper_() = default;
            constexpr reference_wrapper_(T &&t) noexcept : t_(ranges_detail::addressof(t)) {}
            constexpr T &&get() const noexcept { return static_cast<T &&>(*t_); }
        };
    } // namespace ranges_detail
    /// \endcond

    // Can be used to store rvalue references in addition to lvalue references.
    // Also, see: https://wg21.link/lwg2993
    template <typename T> struct reference_wrapper : private ranges_detail::reference_wrapper_<T> {
      private:
        using base_ = ranges_detail::reference_wrapper_<T>;
        using base_::t_;

      public:
        using type = futures::detail::meta::_t<std::remove_reference<T>>;
        using reference = futures::detail::meta::if_<std::is_reference<T>, T, T &>;

        constexpr reference_wrapper() = default;
        template(typename U)(
            /// \pre
            requires(!same_as<uncvref_t<U>, reference_wrapper>) AND constructible_from<
                base_, U>) constexpr reference_wrapper(U &&u) noexcept(std::is_nothrow_constructible<base_, U>::value)
            : ranges_detail::reference_wrapper_<T>{static_cast<U &&>(u)} {}
        constexpr reference get() const noexcept { return this->base_::get(); }
        constexpr operator reference() const noexcept { return get(); }
        template(typename...)(
            /// \pre
            requires(!std::is_rvalue_reference<T>::value)) //
        operator std::reference_wrapper<type>() const noexcept {
            return {get()};
        }
        // clang-format off
        template<typename ...Args>
        constexpr auto CPP_auto_fun(operator())(Args &&...args) (const)
        (
            return invoke(static_cast<reference>(*t_), static_cast<Args &&>(args)...)
        )
        // clang-format on
    };

    struct ref_fn {
        template(typename T)(
            /// \pre
            requires(!is_reference_wrapper_v<T>)) //
            reference_wrapper<T>
            operator()(T &t) const {
            return {t};
        }
        /// \overload
        template <typename T> reference_wrapper<T> operator()(reference_wrapper<T> t) const { return t; }
        /// \overload
        template <typename T> reference_wrapper<T> operator()(std::reference_wrapper<T> t) const { return {t.get()}; }
    };

    /// \ingroup group-functional
    /// \sa `ref_fn`
    RANGES_INLINE_VARIABLE(ref_fn, ref)

    template <typename T> using ref_t = decltype(ref(std::declval<T>()));

    struct unwrap_reference_fn {
        template <typename T> T &&operator()(T &&t) const noexcept { return static_cast<T &&>(t); }
        /// \overload
        template <typename T>
        typename reference_wrapper<T>::reference operator()(reference_wrapper<T> t) const noexcept {
            return t.get();
        }
        /// \overload
        template <typename T> T &operator()(std::reference_wrapper<T> t) const noexcept { return t.get(); }
        /// \overload
        template <typename T> T &operator()(ref_view<T> t) const noexcept { return t.base(); }
    };

    /// \ingroup group-functional
    /// \sa `unwrap_reference_fn`
    RANGES_INLINE_VARIABLE(unwrap_reference_fn, unwrap_reference)

    template <typename T> using unwrap_reference_t = decltype(unwrap_reference(std::declval<T>()));
    /// @}
} // namespace futures::detail

#include <futures/algorithm/detail/traits/range/detail/epilogue.h>

#endif
