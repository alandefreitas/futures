//
// Copyright (c) 2022 alandefreitas (alandefreitas@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//

#ifndef FUTURES_ADAPTOR_DETAIL_UNWRAP_AND_CONTINUE_TRAITS_HPP
#define FUTURES_ADAPTOR_DETAIL_UNWRAP_AND_CONTINUE_TRAITS_HPP

#include <futures/future_options.hpp>
#include <futures/algorithm/traits/is_range.hpp>
#include <futures/traits/future_value.hpp>
#include <futures/detail/move_if_not_shared.hpp>
#include <futures/detail/traits/append_future_option.hpp>
#include <futures/detail/traits/is_tuple.hpp>
#include <futures/adaptor/detail/when_any.hpp>

namespace futures::detail {
    /// Get the element type of a when any result object
    /// This is a very specific helper trait we need
    template <class T, class Enable = void>
    struct range_or_tuple_value {};

    template <class Sequence>
    struct range_or_tuple_value<
        Sequence,
        std::enable_if_t<is_range_v<Sequence>>> {
        using type = range_value_t<Sequence>;
    };

    template <class Sequence>
    struct range_or_tuple_value<
        Sequence,
        std::enable_if_t<detail::is_tuple_v<Sequence>>> {
        using type = std::tuple_element_t<0, Sequence>;
    };

    template <class T>
    using range_or_tuple_value_t = typename range_or_tuple_value<T>::type;

    template <class Future, class Function, class... PrefixArgs>
    struct is_no_unwrap_continuation
        : std::integral_constant<
              bool,
              // clang-format off
              std::is_invocable_v<Function, PrefixArgs..., Future>
              // clang-format on
              > {};

    template <class Future, class Function, class... PrefixArgs>
    constexpr bool is_no_unwrap_continuation_v
        = is_no_unwrap_continuation<Future, Function, PrefixArgs...>::value;

    // -----------------------------------------------------------------
    //
    // is_no_input_continuation
    //

    namespace {
        template <
            bool Enable,
            class Future,
            class Function,
            class... PrefixArgs>
        struct is_no_input_continuation_impl;

        template <class Future, class Function, class... PrefixArgs>
        struct is_no_input_continuation_impl<
            false,
            Future,
            Function,
            PrefixArgs...> : std::false_type {};

        template <class Future, class Function, class... PrefixArgs>
        struct is_no_input_continuation_impl<true, Future, Function, PrefixArgs...>
            : std::integral_constant<
                  bool,
                  // clang-format off
              std::is_invocable_v<Function, PrefixArgs...>
                  // clang-format on
                  > {};
    } // namespace

    template <class Future, class Function, class... PrefixArgs>
    struct is_no_input_continuation
        : is_no_input_continuation_impl<
              // previous unwrap should be false and Future should be a future
              // clang-format off
              is_future_v<Future> &&
              !is_no_unwrap_continuation_v<Future, Function, PrefixArgs...>,
              // clang-format on
              Future,
              Function,
              PrefixArgs...> {};

    template <class Future, class Function, class... PrefixArgs>
    constexpr bool is_no_input_continuation_v
        = is_no_input_continuation<Future, Function, PrefixArgs...>::value;

    // -----------------------------------------------------------------
    //
    // is_value_unwrap_continuation
    //

    namespace {
        template <
            bool Enable,
            class Future,
            class Function,
            class... PrefixArgs>
        struct is_value_unwrap_continuation_impl;

        template <class Future, class Function, class... PrefixArgs>
        struct is_value_unwrap_continuation_impl<
            false,
            Future,
            Function,
            PrefixArgs...> : std::false_type {};

        template <class Future, class Function, class... PrefixArgs>
        struct is_value_unwrap_continuation_impl<
            true,
            Future,
            Function,
            PrefixArgs...>
            : std::integral_constant<
                  bool,
                  // clang-format off
              std::is_invocable_v<Function, PrefixArgs..., future_value_type_t<Future>>
                  // clang-format on
                  > {};
    } // namespace

    template <class Future, class Function, class... PrefixArgs>
    struct is_value_unwrap_continuation
        : is_value_unwrap_continuation_impl<
              // previous unwrap should be false and Future should be a future
              // clang-format off
              is_future_v<Future> &&
              !is_no_unwrap_continuation_v<Future, Function, PrefixArgs...> &&
              !is_no_input_continuation_v<Future, Function, PrefixArgs...>,
              // clang-format on
              Future,
              Function,
              PrefixArgs...> {};

    template <class Future, class Function, class... PrefixArgs>
    constexpr bool is_value_unwrap_continuation_v
        = is_value_unwrap_continuation<Future, Function, PrefixArgs...>::value;

    // -----------------------------------------------------------------
    //
    // is_lvalue_unwrap_continuation
    //

    namespace {
        template <
            bool Enable,
            class Future,
            class Function,
            class... PrefixArgs>
        struct is_lvalue_unwrap_continuation_impl;

        template <class Future, class Function, class... PrefixArgs>
        struct is_lvalue_unwrap_continuation_impl<
            false,
            Future,
            Function,
            PrefixArgs...> : std::false_type {};

        template <class Future, class Function, class... PrefixArgs>
        struct is_lvalue_unwrap_continuation_impl<
            true,
            Future,
            Function,
            PrefixArgs...>
            : std::integral_constant<
                  bool,
                  // clang-format off
              std::is_invocable_v<Function, PrefixArgs..., std::add_lvalue_reference_t<future_value_type_t<Future>>>
                  // clang-format on
                  > {};
    } // namespace

    template <class Future, class Function, class... PrefixArgs>
    struct is_lvalue_unwrap_continuation
        : is_lvalue_unwrap_continuation_impl<
              // previous unwrap should be false and Future should be a future
              // clang-format off
              is_future_v<Future> &&
              !is_no_unwrap_continuation_v<Future, Function, PrefixArgs...> &&
              !is_no_input_continuation_v<Future, Function, PrefixArgs...> &&
              !is_value_unwrap_continuation_v<Future,Function,PrefixArgs...>,
              // clang-format on
              Future,
              Function,
              PrefixArgs...>

    {};

    template <class Future, class Function, class... PrefixArgs>
    constexpr bool is_lvalue_unwrap_continuation_v
        = is_lvalue_unwrap_continuation<Future, Function, PrefixArgs...>::value;

    // -----------------------------------------------------------------
    //
    // is_rvalue_unwrap_continuation
    //

    namespace {
        template <
            bool Enable,
            class Future,
            class Function,
            class... PrefixArgs>
        struct is_rvalue_unwrap_continuation_impl;

        template <class Future, class Function, class... PrefixArgs>
        struct is_rvalue_unwrap_continuation_impl<
            false,
            Future,
            Function,
            PrefixArgs...> : std::false_type {};

        template <class Future, class Function, class... PrefixArgs>
        struct is_rvalue_unwrap_continuation_impl<
            true,
            Future,
            Function,
            PrefixArgs...>
            : std::integral_constant<
                  bool,
                  // clang-format off
              std::is_invocable_v<Function, PrefixArgs..., std::add_rvalue_reference_t<future_value_type_t<Future>>>
                  // clang-format on
                  > {};
    } // namespace

    template <class Future, class Function, class... PrefixArgs>
    struct is_rvalue_unwrap_continuation
        : is_rvalue_unwrap_continuation_impl<
              // previous unwrap should be false and Future should be a future
              // clang-format off
              is_future_v<std::decay_t<Future>> &&
              !is_no_unwrap_continuation_v<Future, Function, PrefixArgs...> &&
              !is_no_input_continuation_v<Future, Function, PrefixArgs...> &&
              !is_value_unwrap_continuation_v<Future,Function, PrefixArgs...> &&
              !is_lvalue_unwrap_continuation_v<Future,Function, PrefixArgs...>,
              // clang-format on
              Future,
              Function,
              PrefixArgs...> {};

    template <class Future, class Function, class... PrefixArgs>
    constexpr bool is_rvalue_unwrap_continuation_v
        = is_rvalue_unwrap_continuation<Future, Function, PrefixArgs...>::value;

    // -----------------------------------------------------------------
    //
    // is_double_unwrap_continuation
    //

    namespace {
        template <
            bool Enable,
            class Future,
            class Function,
            class... PrefixArgs>
        struct is_double_unwrap_continuation_impl;

        template <class Future, class Function, class... PrefixArgs>
        struct is_double_unwrap_continuation_impl<
            false,
            Future,
            Function,
            PrefixArgs...> : std::false_type {};

        template <class T>
        using get_nested_type = typename T::type;

        template <class Future, class Function, class... PrefixArgs>
        struct is_double_unwrap_continuation_impl<
            true,
            Future,
            Function,
            PrefixArgs...>
            : std::integral_constant<
                  bool,
                  // clang-format off
                  is_future_v<std::decay_t<future_value_type_t<Future>>> &&
                  std::is_invocable_v<Function, PrefixArgs..., mp_eval_or<void, get_nested_type, future_value_type<future_value_type_t<Future>>>>
                  // clang-format on
                  > {};
    } // namespace

    template <class Future, class Function, class... PrefixArgs>
    struct is_double_unwrap_continuation
        : is_double_unwrap_continuation_impl<
              // previous unwrap should be false and Future should be a future
              // clang-format off
              is_future_v<std::decay_t<Future>> &&
              !is_no_unwrap_continuation_v<Future, Function, PrefixArgs...> &&
              !is_no_input_continuation_v<Future, Function, PrefixArgs...> &&
              !is_value_unwrap_continuation_v<Future,Function, PrefixArgs...> &&
              !is_lvalue_unwrap_continuation_v<Future,Function, PrefixArgs...> &&
              !is_rvalue_unwrap_continuation_v<Future,Function, PrefixArgs...>,
              // clang-format on
              Future,
              Function,
              PrefixArgs...> {};

    template <class Future, class Function, class... PrefixArgs>
    constexpr bool is_double_unwrap_continuation_v
        = is_double_unwrap_continuation<Future, Function, PrefixArgs...>::value;

    // -----------------------------------------------------------------
    //
    // is_tuple_unwrap_continuation
    //

    namespace {
        template <
            bool Enable,
            class Future,
            class Function,
            class... PrefixArgs>
        struct is_tuple_unwrap_continuation_impl;

        template <class Future, class Function, class... PrefixArgs>
        struct is_tuple_unwrap_continuation_impl<
            false,
            Future,
            Function,
            PrefixArgs...> : std::false_type {};

        template <class Future, class Function, class... PrefixArgs>
        struct is_tuple_unwrap_continuation_impl<
            true,
            Future,
            Function,
            PrefixArgs...>
            : std::integral_constant<
                  bool,
                  // clang-format off
              detail::is_tuple_v<future_value_type_t<Future>>
                  // clang-format on
                  > {};
    } // namespace

    template <class Future, class Function, class... PrefixArgs>
    struct is_tuple_unwrap_continuation
        : is_tuple_unwrap_continuation_impl<
              // previous unwrap should be false and Future should be a future
              // clang-format off
              is_future_v<std::decay_t<Future>> &&
              !is_no_unwrap_continuation_v<Future, Function, PrefixArgs...> &&
              !is_no_input_continuation_v<Future, Function, PrefixArgs...> &&
              !is_value_unwrap_continuation_v<Future,Function, PrefixArgs...> &&
              !is_lvalue_unwrap_continuation_v<Future,Function, PrefixArgs...> &&
              !is_rvalue_unwrap_continuation_v<Future,Function, PrefixArgs...> &&
              !is_double_unwrap_continuation_v<Future,Function, PrefixArgs...>,
              // clang-format on
              Future,
              Function,
              PrefixArgs...>
    // clang-format on
    {};

    template <class Future, class Function, class... PrefixArgs>
    constexpr bool is_tuple_unwrap_continuation_v
        = is_tuple_unwrap_continuation<Future, Function, PrefixArgs...>::value;

    // -----------------------------------------------------------------
    //
    // is_range_unwrap_continuation
    //

    namespace {
        template <
            bool Enable,
            class Future,
            class Function,
            class... PrefixArgs>
        struct is_range_unwrap_continuation_impl;

        template <class Future, class Function, class... PrefixArgs>
        struct is_range_unwrap_continuation_impl<
            false,
            Future,
            Function,
            PrefixArgs...> : std::false_type {};

        template <class Future, class Function, class... PrefixArgs>
        struct is_range_unwrap_continuation_impl<
            true,
            Future,
            Function,
            PrefixArgs...>
            : std::integral_constant<
                  bool,
                  // clang-format off
                  is_range_v<future_value_type_t<Future>>
                  // clang-format on
                  > {};
    } // namespace

    template <class Future, class Function, class... PrefixArgs>
    struct is_range_unwrap_continuation
        : is_range_unwrap_continuation_impl<
              // previous unwrap should be false and Future should be a future
              // clang-format off
              is_future_v<std::decay_t<Future>> &&
              !is_no_unwrap_continuation_v<Future, Function, PrefixArgs...> &&
              !is_no_input_continuation_v<Future, Function, PrefixArgs...> &&
              !is_value_unwrap_continuation_v<Future,Function, PrefixArgs...> &&
              !is_lvalue_unwrap_continuation_v<Future,Function, PrefixArgs...> &&
              !is_rvalue_unwrap_continuation_v<Future,Function, PrefixArgs...> &&
              !is_double_unwrap_continuation_v<Future,Function, PrefixArgs...> &&
              !is_tuple_unwrap_continuation_v<Future,Function, PrefixArgs...>,
              // clang-format on
              Future,
              Function,
              PrefixArgs...>
    // clang-format on
    {};

    template <class Future, class Function, class... PrefixArgs>
    constexpr bool is_range_unwrap_continuation_v
        = is_range_unwrap_continuation<Future, Function, PrefixArgs...>::value;

    // -----------------------------------------------------------------
    //
    // is_when_any_unwrap_continuation
    //

    namespace {
        template <
            bool Enable,
            class Future,
            class Function,
            class... PrefixArgs>
        struct is_when_any_unwrap_continuation_impl;

        template <class Future, class Function, class... PrefixArgs>
        struct is_when_any_unwrap_continuation_impl<
            false,
            Future,
            Function,
            PrefixArgs...> : std::false_type {};

        template <class Future, class Function, class... PrefixArgs>
        struct is_when_any_unwrap_continuation_impl<
            true,
            Future,
            Function,
            PrefixArgs...>
            : std::integral_constant<
                  bool,
                  // clang-format off
                  is_when_any_result_v<future_value_type_t<Future>>
                  // clang-format on
                  > {};
    } // namespace

    template <class Future, class Function, class... PrefixArgs>
    struct is_when_any_unwrap_continuation
        : is_when_any_unwrap_continuation_impl<
              // previous unwrap should be false and Future should be a future
              // clang-format off
              is_future_v<std::decay_t<Future>> &&
              !is_no_unwrap_continuation_v<Future, Function, PrefixArgs...> &&
              !is_no_input_continuation_v<Future, Function, PrefixArgs...> &&
              !is_value_unwrap_continuation_v<Future,Function, PrefixArgs...> &&
              !is_lvalue_unwrap_continuation_v<Future,Function, PrefixArgs...> &&
              !is_rvalue_unwrap_continuation_v<Future,Function, PrefixArgs...> &&
              !is_double_unwrap_continuation_v<Future,Function, PrefixArgs...> &&
              !is_tuple_unwrap_continuation_v<Future,Function, PrefixArgs...> &&
              !is_range_unwrap_continuation_v<Future,Function, PrefixArgs...>,
              // clang-format on
              Future,
              Function,
              PrefixArgs...>
    // clang-format on
    {};

    template <class Future, class Function, class... PrefixArgs>
    constexpr bool is_when_any_unwrap_continuation_v
        = is_when_any_unwrap_continuation<Future, Function, PrefixArgs...>::
            value;

    // -----------------------------------------------------------------
    //
    // is_valid_unwrap_continuation
    //

    template <class Future, class Function, class... PrefixArgs>
    struct is_valid_unwrap_continuation
        : std::integral_constant<
              bool,
              // one of the unwraps should be valid
              // clang-format off
              is_no_unwrap_continuation_v<Future, Function, PrefixArgs...> ||
              is_no_input_continuation_v<Future, Function, PrefixArgs...> ||
              is_value_unwrap_continuation_v<Future,Function, PrefixArgs...> ||
              is_lvalue_unwrap_continuation_v<Future,Function, PrefixArgs...> ||
              is_rvalue_unwrap_continuation_v<Future,Function, PrefixArgs...> ||
              is_double_unwrap_continuation_v<Future,Function, PrefixArgs...> ||
              is_tuple_unwrap_continuation_v<Future,Function, PrefixArgs...> ||
              is_range_unwrap_continuation_v<Future,Function, PrefixArgs...> ||
              is_when_any_unwrap_continuation_v<Future,Function, PrefixArgs...>
              // clang-format on
              > {};

    template <class Future, class Function, class... PrefixArgs>
    constexpr bool is_valid_unwrap_continuation_v
        = is_valid_unwrap_continuation<Future, Function, PrefixArgs...>::value;

    // -----------------------------------------------------------------

} // namespace futures::detail

#endif // FUTURES_ADAPTOR_DETAIL_UNWRAP_AND_CONTINUE_TRAITS_HPP
