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

#ifndef FUTURES_RANGES_RANGE_FWD_HPP
#define FUTURES_RANGES_RANGE_FWD_HPP

#include <type_traits>
#include <utility>

#include <futures/algorithm/detail/traits/range/meta/meta.h>

#include <futures/algorithm/detail/traits/range/concepts/compare.h>
#include <futures/algorithm/detail/traits/range/concepts/concepts.h>

#include <futures/algorithm/detail/traits/range/detail/config.h>
#include <futures/algorithm/detail/traits/range/utility/static_const.h>
#include <futures/algorithm/detail/traits/range/version.h>

/// \defgroup group-iterator Iterator
/// Iterator functionality

/// \defgroup group-iterator-concepts Iterator Concepts
/// \ingroup group-iterator
/// Iterator concepts

/// \defgroup group-range Range
/// Core range functionality

/// \defgroup group-range-concepts Range Concepts
/// \ingroup group-range
/// Range concepts

/// \defgroup group-algorithms Algorithms
/// Iterator- and range-based algorithms, like the standard algorithms

/// \defgroup group-views Views
/// Lazy, non-owning, non-mutating, composable range views

/// \defgroup group-actions Actions
/// Eager, mutating, composable algorithms

/// \defgroup group-utility Utility
/// Utility classes

/// \defgroup group-functional Functional
/// Function and function object utilities

/// \defgroup group-numerics Numerics
/// Numeric utilities

#include <futures/algorithm/detail/traits/range/detail/prologue.h>

RANGES_DIAGNOSTIC_PUSH
RANGES_DIAGNOSTIC_IGNORE_CXX17_COMPAT

namespace futures::detail {
    /// \cond
    namespace views {}

    namespace actions {}

// GCC either fails to accept an attribute on a namespace, or else
// it ignores the deprecation attribute. Frustrating.
#if (RANGES_CXX_VER < RANGES_CXX_STD_17 || defined(__GNUC__) && !defined(__clang__))
    inline namespace v3 {
        using namespace futures::detail;
    }

    namespace view = views;
    namespace action = actions;
#else
    inline namespace RANGES_DEPRECATED("The name futures::detail::v3 namespace is deprecated. "
                                       "Please discontinue using it.") v3 {
        using namespace futures::detail;
    }

    namespace RANGES_DEPRECATED("The futures::detail::view namespace has been renamed to futures::detail::views. "
                                "(Sorry!)") view {
        using namespace views;
    }

    namespace RANGES_DEPRECATED("The futures::detail::action namespace has been renamed to futures::detail::actions. "
                                "(Sorry!)") action {
        using namespace actions;
    }
#endif

    namespace _end_ {
        struct fn;
    }
    using end_fn = _end_::fn;

    namespace _size_ {
        struct fn;
    }

    template <typename> struct result_of;

    template <typename Sig>
    using result_of_t RANGES_DEPRECATED("futures::detail::result_of_t is deprecated. "
                                        "Please use futures::detail::invoke_result_t") = futures::detail::meta::_t<result_of<Sig>>;
    /// \endcond

    template <typename...> struct variant;

    struct dangling;

    struct make_pipeable_fn;

    struct pipeable_base;

    template <typename First, typename Second> struct composed;

    template <typename... Fns> struct overloaded;

    namespace actions {
        template <typename ActionFn> struct action_closure;
    }

    namespace views {
        template <typename ViewFn> struct view_closure;
    }

    struct advance_fn;

    struct advance_to_fn;

    struct advance_bounded_fn;

    struct next_fn;

    struct prev_fn;

    struct distance_fn;

    struct iter_size_fn;

    template <typename T> struct indirectly_readable_traits;

    template <typename T>
    using readable_traits
        RANGES_DEPRECATED("Please use futures::detail::indirectly_readable_traits") = indirectly_readable_traits<T>;

    template <typename T> struct incrementable_traits;

    struct view_base {};

    /// \cond
    namespace ranges_detail {
        template <typename T> struct difference_type_;

        template <typename T> struct value_type_;
    } // namespace ranges_detail

    template <typename T>
    using difference_type
        RANGES_DEPRECATED("futures::detail::difference_type<T>::type is deprecated. Use "
                          "futures::detail::incrementable_traits<T>::difference_type instead.") = ranges_detail::difference_type_<T>;

    template <typename T>
    using value_type
        RANGES_DEPRECATED("futures::detail::value_type<T>::type is deprecated. Use "
                          "futures::detail::indirectly_readable_traits<T>::value_type instead.") = ranges_detail::value_type_<T>;

    template <typename T> struct size_type;
    /// \endcond

    /// \cond
    namespace ranges_detail {
        struct ignore_t {
            ignore_t() = default;
            template <typename T> constexpr ignore_t(T &&) noexcept {}
            template <typename T> constexpr ignore_t const &operator=(T &&) const noexcept { return *this; }
        };

        struct value_init {
            template <typename T> operator T() const { return T{}; }
        };

        struct make_compressed_pair_fn;

        template <typename T> constexpr futures::detail::meta::_t<std::remove_reference<T>> &&move(T &&t) noexcept {
            return static_cast<futures::detail::meta::_t<std::remove_reference<T>> &&>(t);
        }

        struct as_const_fn {
            template <typename T> constexpr T const &operator()(T &t) const noexcept { return t; }
            template <typename T> constexpr T const &&operator()(T &&t) const noexcept { return (T &&) t; }
        };

        RANGES_INLINE_VARIABLE(as_const_fn, as_const)

        template <typename T> using as_const_t = decltype(as_const(std::declval<T>()));

        template <typename T> using decay_t = futures::detail::meta::_t<std::decay<T>>;

        template <typename T, typename R = futures::detail::meta::_t<std::remove_reference<T>>>
        using as_ref_t = futures::detail::meta::_t<std::add_lvalue_reference<futures::detail::meta::_t<std::remove_const<R>>>>;

        template <typename T, typename R = futures::detail::meta::_t<std::remove_reference<T>>>
        using as_cref_t = futures::detail::meta::_t<std::add_lvalue_reference<R const>>;

        struct get_first;
        struct get_second;

        template <typename Val1, typename Val2> struct replacer_fn;

        template <typename Pred, typename Val> struct replacer_if_fn;

        template <typename I> struct move_into_cursor;

        template <typename Int> struct from_end_;

        template <typename... Ts> constexpr int ignore_unused(Ts &&...) { return 42; }

        template <int I> struct priority_tag : priority_tag<I - 1> {};

        template <> struct priority_tag<0> {};

#if defined(__clang__) && !defined(_LIBCPP_VERSION)
        template <typename T, typename... Args>
        RANGES_INLINE_VAR constexpr bool is_trivially_constructible_v = __is_trivially_constructible(T, Args...);
        template <typename T>
        RANGES_INLINE_VAR constexpr bool is_trivially_default_constructible_v = is_trivially_constructible_v<T>;
        template <typename T>
        RANGES_INLINE_VAR constexpr bool is_trivially_copy_constructible_v = is_trivially_constructible_v<T, T const &>;
        template <typename T>
        RANGES_INLINE_VAR constexpr bool is_trivially_move_constructible_v = is_trivially_constructible_v<T, T>;
        template <typename T> RANGES_INLINE_VAR constexpr bool is_trivially_copyable_v = __is_trivially_copyable(T);
        template <typename T, typename U>
        RANGES_INLINE_VAR constexpr bool is_trivially_assignable_v = __is_trivially_assignable(T, U);
        template <typename T>
        RANGES_INLINE_VAR constexpr bool is_trivially_copy_assignable_v = is_trivially_assignable_v<T &, T const &>;
        template <typename T>
        RANGES_INLINE_VAR constexpr bool is_trivially_move_assignable_v = is_trivially_assignable_v<T &, T>;

        template <typename T, typename... Args>
        struct is_trivially_constructible : futures::detail::meta::bool_<is_trivially_constructible_v<T, Args...>> {};
        template <typename T>
        struct is_trivially_default_constructible : futures::detail::meta::bool_<is_trivially_default_constructible_v<T>> {};
        template <typename T>
        struct is_trivially_copy_constructible : futures::detail::meta::bool_<is_trivially_copy_constructible_v<T>> {};
        template <typename T>
        struct is_trivially_move_constructible : futures::detail::meta::bool_<is_trivially_move_constructible_v<T>> {};
        template <typename T> struct is_trivially_copyable : futures::detail::meta::bool_<is_trivially_copyable_v<T>> {};
        template <typename T, typename U>
        struct is_trivially_assignable : futures::detail::meta::bool_<is_trivially_assignable_v<T, U>> {};
        template <typename T> struct is_trivially_copy_assignable : futures::detail::meta::bool_<is_trivially_copy_assignable_v<T>> {};
        template <typename T> struct is_trivially_move_assignable : futures::detail::meta::bool_<is_trivially_move_assignable_v<T>> {};
#else
        using std::is_trivially_assignable;
        using std::is_trivially_constructible;
        using std::is_trivially_copy_assignable;
        using std::is_trivially_copy_constructible;
        using std::is_trivially_copyable;
        using std::is_trivially_default_constructible;
        using std::is_trivially_move_assignable;
        using std::is_trivially_move_constructible;
#if META_CXX_TRAIT_VARIABLE_TEMPLATES
        using std::is_trivially_assignable_v;
        using std::is_trivially_constructible_v;
        using std::is_trivially_copy_assignable_v;
        using std::is_trivially_copy_constructible_v;
        using std::is_trivially_copyable_v;
        using std::is_trivially_default_constructible_v;
        using std::is_trivially_move_assignable_v;
        using std::is_trivially_move_constructible_v;
#else
        template <typename T, typename... Args>
        RANGES_INLINE_VAR constexpr bool is_trivially_constructible_v = is_trivially_constructible<T, Args...>::value;
        template <typename T>
        RANGES_INLINE_VAR constexpr bool is_trivially_default_constructible_v =
            is_trivially_default_constructible<T>::value;
        template <typename T>
        RANGES_INLINE_VAR constexpr bool is_trivially_copy_constructible_v = is_trivially_copy_constructible<T>::value;
        template <typename T>
        RANGES_INLINE_VAR constexpr bool is_trivially_move_constructible_v = is_trivially_move_constructible<T>::value;
        template <typename T>
        RANGES_INLINE_VAR constexpr bool is_trivially_copyable_v = is_trivially_copyable<T>::value;
        template <typename T, typename U>
        RANGES_INLINE_VAR constexpr bool is_trivially_assignable_v = is_trivially_assignable<T, U>::value;
        template <typename T>
        RANGES_INLINE_VAR constexpr bool is_trivially_copy_assignable_v = is_trivially_copy_assignable<T>::value;
        template <typename T>
        RANGES_INLINE_VAR constexpr bool is_trivially_move_assignable_v = is_trivially_move_assignable<T>::value;
#endif
#endif

        template <typename T>
        RANGES_INLINE_VAR constexpr bool is_trivial_v =
            is_trivially_copyable_v<T> &&is_trivially_default_constructible_v<T>;

        template <typename T> struct is_trivial : futures::detail::meta::bool_<is_trivial_v<T>> {};

#if RANGES_CXX_LIB_IS_FINAL > 0
#if defined(__clang__) && !defined(_LIBCPP_VERSION)
        template <typename T> RANGES_INLINE_VAR constexpr bool is_final_v = __is_final(T);

        template <typename T> struct is_final : futures::detail::meta::bool_<is_final_v<T>> {};
#else
        using std::is_final;
#if META_CXX_TRAIT_VARIABLE_TEMPLATES
        using std::is_final_v;
#else
        template <typename T> RANGES_INLINE_VAR constexpr bool is_final_v = is_final<T>::value;
#endif
#endif
#else
        template <typename T> RANGES_INLINE_VAR constexpr bool is_final_v = false;

        template <typename T> using is_final = std::false_type;
#endif

        // Work around libc++'s buggy std::is_function
        // Function types here:
        template <typename T> char (&is_function_impl_(priority_tag<0>))[1];

        // Array types here:
        template <typename T, typename = decltype((*(T *)0)[0])> char (&is_function_impl_(priority_tag<1>))[2];

        // Anything that can be returned from a function here (including
        // void and reference types):
        template <typename T, typename = T (*)()> char (&is_function_impl_(priority_tag<2>))[3];

        // Classes and unions (including abstract types) here:
        template <typename T, typename = int T::*> char (&is_function_impl_(priority_tag<3>))[4];

        template <typename T>
        RANGES_INLINE_VAR constexpr bool is_function_v = sizeof(ranges_detail::is_function_impl_<T>(priority_tag<3>{})) == 1;

        template <typename T> struct remove_rvalue_reference { using type = T; };

        template <typename T> struct remove_rvalue_reference<T &&> { using type = T; };

        template <typename T> using remove_rvalue_reference_t = futures::detail::meta::_t<remove_rvalue_reference<T>>;

        // Workaround bug in the Standard Library:
        // From cannot be an incomplete class type despite that
        // is_convertible<X, Y> should be equivalent to is_convertible<X&&, Y>
        // in such a case.
        template <typename From, typename To>
        using is_convertible = std::is_convertible<futures::detail::meta::_t<std::add_rvalue_reference<From>>, To>;
    } // namespace ranges_detail
    /// \endcond

    struct begin_tag {};
    struct end_tag {};
    struct copy_tag {};
    struct move_tag {};

    template <typename T> using uncvref_t = futures::detail::meta::_t<std::remove_cv<futures::detail::meta::_t<std::remove_reference<T>>>>;

    struct not_equal_to;
    struct equal_to;
    struct less;
#if __cplusplus > 201703L && defined(__cpp_impl_three_way_comparison) && __has_include(<compare>)
    struct compare_three_way;
#endif // __cplusplus
    struct identity;
    template <typename Pred> struct logical_negate;

    enum cardinality : std::ptrdiff_t { infinite = -3, unknown = -2, finite = -1 };

    template <typename Rng, typename Void = void> struct range_cardinality;

    template <typename Rng> using is_finite = futures::detail::meta::bool_<range_cardinality<Rng>::value >= finite>;

    template <typename Rng> using is_infinite = futures::detail::meta::bool_<range_cardinality<Rng>::value == infinite>;

    template <typename S, typename I> RANGES_INLINE_VAR constexpr bool disable_sized_sentinel = false;

    template <typename R> RANGES_INLINE_VAR constexpr bool enable_borrowed_range = false;

    namespace ranges_detail {
        template <typename R>
        RANGES_DEPRECATED("Please use futures::detail::enable_borrowed_range instead.")
        RANGES_INLINE_VAR constexpr bool enable_safe_range = enable_borrowed_range<R>;
    } // namespace ranges_detail

    using ranges_detail::enable_safe_range;

    template <typename Cur> struct basic_mixin;

    template <typename Cur> struct RANGES_EMPTY_BASES basic_iterator;

    template <cardinality> struct basic_view : view_base {};

    template <typename Derived, cardinality C = finite> struct view_facade;

    template <typename Derived, typename BaseRng, cardinality C = range_cardinality<BaseRng>::value>
    struct view_adaptor;

    template <typename I, typename S> struct common_iterator;

    /// \cond
    namespace ranges_detail {
        template <typename I> struct cpp17_iterator_cursor;

        template <typename I> using cpp17_iterator = basic_iterator<cpp17_iterator_cursor<I>>;
    } // namespace ranges_detail
    /// \endcond

    template <typename First, typename Second> struct compressed_pair;

    template <typename T> struct bind_element;

    template <typename T> using bind_element_t = futures::detail::meta::_t<bind_element<T>>;

    template <typename Derived, cardinality = finite> struct view_interface;

    template <typename T> struct istream_view;

    template <typename I, typename S = I> struct RANGES_EMPTY_BASES iterator_range;

    template <typename I, typename S = I> struct sized_iterator_range;

    template <typename T> struct reference_wrapper;

    // Views
    //
    template <typename Rng, typename Pred> struct RANGES_EMPTY_BASES adjacent_filter_view;

    namespace views {
        struct adjacent_filter_fn;
    }

    template <typename Rng, typename Pred> struct RANGES_EMPTY_BASES adjacent_remove_if_view;

    namespace views {
        struct adjacent_remove_if_fn;
    }

    namespace views {
        struct all_fn;
    }

    template <typename Rng> struct const_view;

    namespace views {
        struct const_fn;
    }

    template <typename I> struct counted_view;

    namespace views {
        struct counted_fn;
    }

    struct default_sentinel_t;

    template <typename I> struct move_iterator;

    template <typename I> using move_into_iterator = basic_iterator<ranges_detail::move_into_cursor<I>>;

    template <typename Rng, bool = (bool)is_infinite<Rng>()> struct RANGES_EMPTY_BASES cycled_view;

    namespace views {
        struct cycle_fn;
    }

    /// \cond
    namespace ranges_detail {
        template <typename I> struct reverse_cursor;
    }
    /// \endcond

    template <typename I> using reverse_iterator = basic_iterator<ranges_detail::reverse_cursor<I>>;

    template <typename T> struct empty_view;

    namespace views {
        struct empty_fn;
    }

    template <typename Rng, typename Fun> struct group_by_view;

    namespace views {
        struct group_by_fn;
    }

    template <typename Rng> struct indirect_view;

    namespace views {
        struct indirect_fn;
    }

    struct unreachable_sentinel_t;

    template <typename From, typename To = unreachable_sentinel_t> struct iota_view;

    template <typename From, typename To = From> struct closed_iota_view;

    namespace views {
        struct iota_fn;
        struct closed_iota_fn;
    } // namespace views

    template <typename Rng> struct join_view;

    template <typename Rng, typename ValRng> struct join_with_view;

    namespace views {
        struct join_fn;
    }

    template <typename... Rngs> struct concat_view;

    namespace views {
        struct concat_fn;
    }

    template <typename Rng, typename Fun> struct partial_sum_view;

    namespace views {
        struct partial_sum_fn;
    }

    template <typename Rng> struct move_view;

    namespace views {
        struct move_fn;
    }

    template <typename Rng> struct ref_view;

    namespace views {
        struct ref_fn;
    }

    template <typename Val> struct repeat_view;

    namespace views {
        struct repeat_fn;
    }

    template <typename Rng> struct RANGES_EMPTY_BASES reverse_view;

    namespace views {
        struct reverse_fn;
    }

    template <typename Rng> struct slice_view;

    namespace views {
        struct slice_fn;
    }

    // template<typename Rng, typename Fun>
    // struct split_view;

    // namespace views
    // {
    //     struct split_fn;
    // }

    template <typename Rng> struct single_view;

    namespace views {
        struct single_fn;
    }

    template <typename Rng> struct stride_view;

    namespace views {
        struct stride_fn;
    }

    template <typename Rng> struct take_view;

    namespace views {
        struct take_fn;
    }

    /// \cond
    namespace ranges_detail {
        template <typename Rng> struct is_random_access_common_;

        template <typename Rng, bool IsRandomAccessCommon = is_random_access_common_<Rng>::value>
        struct take_exactly_view_;
    } // namespace ranges_detail
    /// \endcond

    template <typename Rng> using take_exactly_view = ranges_detail::take_exactly_view_<Rng>;

    namespace views {
        struct take_exactly_fn;
    }

    template <typename Rng, typename Pred> struct iter_take_while_view;

    template <typename Rng, typename Pred> struct take_while_view;

    namespace views {
        struct iter_take_while_fn;
        struct take_while_fn;
    } // namespace views

    template <typename Rng, typename Regex, typename SubMatchRange> struct tokenize_view;

    namespace views {
        struct tokenize_fn;
    }

    template <typename Rng, typename Fun> struct iter_transform_view;

    template <typename Rng, typename Fun> struct transform_view;

    namespace views {
        struct transform_fn;
    }

    template <typename Rng, typename Val1, typename Val2>
    using replace_view = iter_transform_view<Rng, ranges_detail::replacer_fn<Val1, Val2>>;

    template <typename Rng, typename Pred, typename Val>
    using replace_if_view = iter_transform_view<Rng, ranges_detail::replacer_if_fn<Pred, Val>>;

    namespace views {
        struct replace_fn;

        struct replace_if_fn;
    } // namespace views

    template <typename Rng, typename Pred> struct trim_view;

    namespace views {
        struct trim_fn;
    }

    template <typename I> struct unbounded_view;

    namespace views {
        struct unbounded_fn;
    }

    template <typename Rng> using unique_view = adjacent_filter_view<Rng, logical_negate<equal_to>>;

    namespace views {
        struct unique_fn;
    }

    template <typename Rng> using keys_range_view = transform_view<Rng, ranges_detail::get_first>;

    template <typename Rng> using values_view = transform_view<Rng, ranges_detail::get_second>;

    namespace views {
        struct keys_fn;

        struct values_fn;
    } // namespace views

    template <typename Fun, typename... Rngs> struct iter_zip_with_view;

    template <typename Fun, typename... Rngs> struct zip_with_view;

    template <typename... Rngs> struct zip_view;

    namespace views {
        struct iter_zip_with_fn;

        struct zip_with_fn;

        struct zip_fn;
    } // namespace views
} // namespace futures::detail

/// \cond
namespace futures::detail {
    // namespace futures::detail::concepts = ::futures::detail::concepts;
    using namespace ::futures::detail::concepts::defs;
    using ::futures::detail::concepts::and_v;
} // namespace futures::detail
/// \endcond

RANGES_DIAGNOSTIC_POP

#include <futures/algorithm/detail/traits/range/detail/epilogue.h>

#endif
