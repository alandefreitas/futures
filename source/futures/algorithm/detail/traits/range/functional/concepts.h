
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
#ifndef FUTURES_RANGES_FUNCTIONAL_CONCEPTS_HPP
#define FUTURES_RANGES_FUNCTIONAL_CONCEPTS_HPP

#include <futures/algorithm/detail/traits/range/concepts/concepts.h>

#include <futures/algorithm/detail/traits/range/functional/invoke.h>

#include <futures/algorithm/detail/traits/range/detail/prologue.h>

namespace futures::detail {
    /// \addtogroup group-functional
    /// @{

    // clang-format off
    // WORKAROUND mysterious msvc bug
#if defined(_MSC_VER)
    template<typename Fun, typename... Args>
    CPP_concept invocable =
        std::is_invocable_v<Fun, Args...>;
#else
    template<typename Fun, typename... Args>
    CPP_requires(invocable_,
        requires(Fun && fn) //
        (
            invoke((Fun &&) fn, std::declval<Args>()...)
        ));
    template<typename Fun, typename... Args>
    CPP_concept invocable =
        CPP_requires_ref(::futures::detail::invocable_, Fun, Args...);
#endif

    template<typename Fun, typename... Args>
    CPP_concept regular_invocable =
        invocable<Fun, Args...>;
        // Axiom: equality_preserving(invoke(f, args...))

    template<typename Fun, typename... Args>
    CPP_requires(predicate_,
        requires(Fun && fn) //
        (
            concepts::requires_<
                convertible_to<
                    decltype(invoke((Fun &&) fn, std::declval<Args>()...)),
                    bool>>
        ));
    template<typename Fun, typename... Args>
    CPP_concept predicate =
        regular_invocable<Fun, Args...> &&
        CPP_requires_ref(::futures::detail::predicate_, Fun, Args...);

    template<typename R, typename T, typename U>
    CPP_concept relation =
        predicate<R, T, T> &&
        predicate<R, U, U> &&
        predicate<R, T, U> &&
        predicate<R, U, T>;

    template<typename R, typename T, typename U>
    CPP_concept strict_weak_order =
        relation<R, T, U>;
    // clang-format on

    namespace cpp20 {
        using ::futures::detail::invocable;
        using ::futures::detail::predicate;
        using ::futures::detail::regular_invocable;
        using ::futures::detail::relation;
        using ::futures::detail::strict_weak_order;
    } // namespace cpp20
    /// @}
} // namespace futures::detail

#include <futures/algorithm/detail/traits/range/detail/epilogue.h>

#endif
