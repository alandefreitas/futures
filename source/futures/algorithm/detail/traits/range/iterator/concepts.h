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

#ifndef FUTURES_RANGES_ITERATOR_CONCEPTS_HPP
#define FUTURES_RANGES_ITERATOR_CONCEPTS_HPP

#include <iterator>
#include <type_traits>

#include <futures/algorithm/detail/traits/range/meta/meta.h>

#include <futures/algorithm/detail/traits/range/concepts/concepts.h>

#include <futures/algorithm/detail/traits/range/range_fwd.h>

#include <futures/algorithm/detail/traits/range/functional/comparisons.h>
#include <futures/algorithm/detail/traits/range/functional/concepts.h>
#include <futures/algorithm/detail/traits/range/functional/identity.h>
#include <futures/algorithm/detail/traits/range/functional/invoke.h>
#include <futures/algorithm/detail/traits/range/iterator/access.h>
#include <futures/algorithm/detail/traits/range/iterator/traits.h>

#ifdef _GLIBCXX_DEBUG
#include <debug/safe_iterator.h>
#endif

#include <futures/algorithm/detail/traits/range/detail/prologue.h>

namespace futures::detail {
    /// \addtogroup group-iterator-concepts
    /// @{

    /// \cond
    namespace ranges_detail {
        template <typename I>
        using iter_traits_t = futures::detail::meta::conditional_t<is_std_iterator_traits_specialized_v<I>, std::iterator_traits<I>, I>;

#if defined(_GLIBCXX_DEBUG)
        template(typename I, typename T, typename Seq)(
            /// \pre
            requires same_as<
                I, __gnu_debug::_Safe_iterator<T *, Seq>>) auto iter_concept_(__gnu_debug::_Safe_iterator<T *, Seq>,
                                                                              priority_tag<3>)
            -> ::futures::detail::contiguous_iterator_tag
#endif
#if defined(__GLIBCXX__)
            template(typename I, typename T, typename Seq)(
                /// \pre
                requires same_as<I, __gnu_cxx::__normal_iterator<
                                        T *, Seq>>) auto iter_concept_(__gnu_cxx::__normal_iterator<T *, Seq>,
                                                                       priority_tag<3>)
                -> ::futures::detail::contiguous_iterator_tag;
#endif
#if defined(_LIBCPP_VERSION)
        template(typename I, typename T)(
            /// \pre
            requires same_as<I, std::__wrap_iter<T *>>) auto iter_concept_(std::__wrap_iter<T *>, priority_tag<3>)
            -> ::futures::detail::contiguous_iterator_tag;
#endif
#if defined(_MSVC_STL_VERSION) || defined(_IS_WRS)
        template(typename I)(
            /// \pre
            requires same_as<I, class I::_Array_iterator>) auto iter_concept_(I, priority_tag<3>)
            -> ::futures::detail::contiguous_iterator_tag;
        template(typename I)(
            /// \pre
            requires same_as<I, class I::_Array_const_iterator>) auto iter_concept_(I, priority_tag<3>)
            -> ::futures::detail::contiguous_iterator_tag;
        template(typename I)(
            /// \pre
            requires same_as<I, class I::_Vector_iterator>) auto iter_concept_(I, priority_tag<3>)
            -> ::futures::detail::contiguous_iterator_tag;
        template(typename I)(
            /// \pre
            requires same_as<I, class I::_Vector_const_iterator>) auto iter_concept_(I, priority_tag<3>)
            -> ::futures::detail::contiguous_iterator_tag;
        template(typename I)(
            /// \pre
            requires same_as<I, class I::_String_iterator>) auto iter_concept_(I, priority_tag<3>)
            -> ::futures::detail::contiguous_iterator_tag;
        template(typename I)(
            /// \pre
            requires same_as<I, class I::_String_const_iterator>) auto iter_concept_(I, priority_tag<3>)
            -> ::futures::detail::contiguous_iterator_tag;
        template(typename I)(
            /// \pre
            requires same_as<I, class I::_String_view_iterator>) auto iter_concept_(I, priority_tag<3>)
            -> ::futures::detail::contiguous_iterator_tag;
#endif
        template(typename I, typename T)(
            /// \pre
            requires same_as<I, T *>) auto iter_concept_(T *, priority_tag<3>) -> ::futures::detail::contiguous_iterator_tag;
        template <typename I> auto iter_concept_(I, priority_tag<2>) -> typename iter_traits_t<I>::iterator_concept;
        template <typename I> auto iter_concept_(I, priority_tag<1>) -> typename iter_traits_t<I>::iterator_category;
        template <typename I>
        auto iter_concept_(I, priority_tag<0>)
            -> enable_if_t<!is_std_iterator_traits_specialized_v<I>, std::random_access_iterator_tag>;

        template <typename I> using iter_concept_t = decltype(iter_concept_<I>(std::declval<I>(), priority_tag<3>{}));

        using ::futures::detail::concepts::ranges_detail::weakly_equality_comparable_with_;

        template <typename I>
        using readable_types_t = futures::detail::meta::list<iter_value_t<I>, iter_reference_t<I>, iter_rvalue_reference_t<I>>;
    } // namespace ranges_detail
      /// \endcond

    // clang-format off
    template(typename I)(
    concept (readable_)(I),
        // requires (I const i)
        // (
        //     { *i } -> same_as<iter_reference_t<I>>;
        //     { iter_move(i) } -> same_as<iter_rvalue_reference_t<I>>;
        // ) &&
        same_as<iter_reference_t<I const>, iter_reference_t<I>> AND
        same_as<iter_rvalue_reference_t<I const>, iter_rvalue_reference_t<I>> AND
        common_reference_with<iter_reference_t<I> &&, iter_value_t<I> &> AND
        common_reference_with<iter_reference_t<I> &&,
                              iter_rvalue_reference_t<I> &&> AND
        common_reference_with<iter_rvalue_reference_t<I> &&, iter_value_t<I> const &>
    );

    template<typename I>
    CPP_concept indirectly_readable = //
        CPP_concept_ref(::futures::detail::readable_, uncvref_t<I>);

    template<typename I>
    RANGES_DEPRECATED("Please use ::futures::detail::indirectly_readable instead")
    RANGES_INLINE_VAR constexpr bool readable = //
        indirectly_readable<I>;

    template<typename O, typename T>
    CPP_requires(writable_,
        requires(O && o, T && t) //
        (
            *o = (T &&) t,
            *(O &&) o = (T &&) t,
            const_cast<iter_reference_t<O> const &&>(*o) = (T &&) t,
            const_cast<iter_reference_t<O> const &&>(*(O &&) o) = (T &&) t
        ));
    template<typename O, typename T>
    CPP_concept indirectly_writable = //
        CPP_requires_ref(::futures::detail::writable_, O, T);

    template<typename O, typename T>
    RANGES_DEPRECATED("Please use ::futures::detail::indirectly_writable instead")
    RANGES_INLINE_VAR constexpr bool writable = //
        indirectly_writable<O, T>;
    // clang-format on

    /// \cond
    namespace ranges_detail {
#if RANGES_CXX_INLINE_VARIABLES >= RANGES_CXX_INLINE_VARIABLES_17
        template <typename D> inline constexpr bool _is_integer_like_ = std::is_integral<D>::value;
#else
        template <typename D, typename = void> constexpr bool _is_integer_like_ = std::is_integral<D>::value;
#endif

        // gcc10 uses for std::futures::detail::range_difference_t<
        // std::futures::detail::iota_view<size_t, size_t>> == __int128
#if __SIZEOF_INT128__
        __extension__ typedef __int128 int128_t;
#if RANGES_CXX_INLINE_VARIABLES >= RANGES_CXX_INLINE_VARIABLES_17
        template <> inline constexpr bool _is_integer_like_<int128_t> = true;
#else
        template <typename Enable> constexpr bool _is_integer_like_<int128_t, Enable> = true;
#endif
#endif // __SIZEOF_INT128__

        // clang-format off
        template<typename D>
        CPP_concept integer_like_ = _is_integer_like_<D>;

#ifdef RANGES_WORKAROUND_MSVC_792338
        template<typename D, bool Signed = (D(-1) < D(0))>
        constexpr bool _is_signed_(D *)
        {
            return Signed;
        }
        constexpr bool _is_signed_(void *)
        {
            return false;
        }

        template<typename D>
        CPP_concept signed_integer_like_ =
            integer_like_<D> && ranges_detail::_is_signed_((D*) nullptr);
#else // ^^^ workaround / no workaround vvv
        template(typename D)(
        concept (signed_integer_like_impl_)(D),
            integer_like_<D> AND
            concepts::type<std::integral_constant<bool, (D(-1) < D(0))>> AND
            std::integral_constant<bool, (D(-1) < D(0))>::value
        );

        template<typename D>
        CPP_concept signed_integer_like_ =
            integer_like_<D> &&
            CPP_concept_ref(ranges_detail::signed_integer_like_impl_, D);
#endif // RANGES_WORKAROUND_MSVC_792338
       // clang-format on
    } // namespace ranges_detail
      /// \endcond

    // clang-format off
    template<typename I>
    CPP_requires(weakly_incrementable_,
        requires(I i) //
        (
            ++i,
            i++,
            concepts::requires_<same_as<I&, decltype(++i)>>
        ));

    template(typename I)(
    concept (weakly_incrementable_)(I),
        concepts::type<iter_difference_t<I>> AND
        ranges_detail::signed_integer_like_<iter_difference_t<I>>);

    template<typename I>
    CPP_concept weakly_incrementable =
        semiregular<I> &&
        CPP_requires_ref(::futures::detail::weakly_incrementable_, I) &&
        CPP_concept_ref(::futures::detail::weakly_incrementable_, I);

    template<typename I>
    CPP_requires(incrementable_,
        requires(I i) //
        (
            concepts::requires_<same_as<I, decltype(i++)>>
        ));
    template<typename I>
    CPP_concept incrementable =
        regular<I> &&
        weakly_incrementable<I> &&
        CPP_requires_ref(::futures::detail::incrementable_, I);

    template(typename I)(
    concept (input_or_output_iterator_)(I),
        ranges_detail::dereferenceable_<I&>
    );

    template<typename I>
    CPP_concept input_or_output_iterator =
        weakly_incrementable<I> &&
        CPP_concept_ref(::futures::detail::input_or_output_iterator_, I);

    template<typename S, typename I>
    CPP_concept sentinel_for =
        semiregular<S> &&
        input_or_output_iterator<I> &&
        ranges_detail::weakly_equality_comparable_with_<S, I>;

    template<typename S, typename I>
    CPP_requires(sized_sentinel_for_,
        requires(S const & s, I const & i) //
        (
            s - i,
            i - s,
            concepts::requires_<same_as<iter_difference_t<I>, decltype(s - i)>>,
            concepts::requires_<same_as<iter_difference_t<I>, decltype(i - s)>>
        ));
    template(typename S, typename I)(
    concept (sized_sentinel_for_)(S, I),
        (!disable_sized_sentinel<std::remove_cv_t<S>, std::remove_cv_t<I>>) AND
        sentinel_for<S, I>);

    template<typename S, typename I>
    CPP_concept sized_sentinel_for =
        CPP_concept_ref(sized_sentinel_for_, S, I) &&
        CPP_requires_ref(::futures::detail::sized_sentinel_for_, S, I);

    template<typename Out, typename T>
    CPP_requires(output_iterator_,
        requires(Out o, T && t) //
        (
            *o++ = (T &&) t
        ));
    template<typename Out, typename T>
    CPP_concept output_iterator =
        input_or_output_iterator<Out> &&
        indirectly_writable<Out, T> &&
        CPP_requires_ref(::futures::detail::output_iterator_, Out, T);

    template(typename I, typename Tag)(
    concept (with_category_)(I, Tag),
        derived_from<ranges_detail::iter_concept_t<I>, Tag>
    );

    template<typename I>
    CPP_concept input_iterator =
        input_or_output_iterator<I> &&
        indirectly_readable<I> &&
        CPP_concept_ref(::futures::detail::with_category_, I, std::input_iterator_tag);

    template<typename I>
    CPP_concept forward_iterator =
        input_iterator<I> &&
        incrementable<I> &&
        sentinel_for<I, I> &&
        CPP_concept_ref(::futures::detail::with_category_, I, std::forward_iterator_tag);

    template<typename I>
    CPP_requires(bidirectional_iterator_,
        requires(I i) //
        (
            --i,
            i--,
            concepts::requires_<same_as<I&, decltype(--i)>>,
            concepts::requires_<same_as<I, decltype(i--)>>
        ));
    template<typename I>
    CPP_concept bidirectional_iterator =
        forward_iterator<I> &&
        CPP_requires_ref(::futures::detail::bidirectional_iterator_, I) &&
        CPP_concept_ref(::futures::detail::with_category_, I, std::bidirectional_iterator_tag);

    template<typename I>
    CPP_requires(random_access_iterator_,
        requires(I i, iter_difference_t<I> n)
        (
            i + n,
            n + i,
            i - n,
            i += n,
            i -= n,
            concepts::requires_<same_as<decltype(i + n), I>>,
            concepts::requires_<same_as<decltype(n + i), I>>,
            concepts::requires_<same_as<decltype(i - n), I>>,
            concepts::requires_<same_as<decltype(i += n), I&>>,
            concepts::requires_<same_as<decltype(i -= n), I&>>,
            concepts::requires_<same_as<decltype(i[n]), iter_reference_t<I>>>
        ));
    template<typename I>
    CPP_concept random_access_iterator =
        bidirectional_iterator<I> &&
        totally_ordered<I> &&
        sized_sentinel_for<I, I> &&
        CPP_requires_ref(::futures::detail::random_access_iterator_, I) &&
        CPP_concept_ref(::futures::detail::with_category_, I, std::random_access_iterator_tag);

    template(typename I)(
    concept (contiguous_iterator_)(I),
        std::is_lvalue_reference<iter_reference_t<I>>::value AND
        same_as<iter_value_t<I>, uncvref_t<iter_reference_t<I>>> AND
        derived_from<ranges_detail::iter_concept_t<I>, ::futures::detail::contiguous_iterator_tag>
    );

    template<typename I>
    CPP_concept contiguous_iterator =
        random_access_iterator<I> &&
        CPP_concept_ref(::futures::detail::contiguous_iterator_, I);
    // clang-format on

    /////////////////////////////////////////////////////////////////////////////////////
    // iterator_tag_of
    template <typename Rng>
    using iterator_tag_of =                              //
        std::enable_if_t<                                //
            input_iterator<Rng>,                         //
            futures::detail::meta::conditional_t<                         //
                contiguous_iterator<Rng>,                //
                ::futures::detail::contiguous_iterator_tag,         //
                futures::detail::meta::conditional_t<                     //
                    random_access_iterator<Rng>,         //
                    std::random_access_iterator_tag,     //
                    futures::detail::meta::conditional_t<                 //
                        bidirectional_iterator<Rng>,     //
                        std::bidirectional_iterator_tag, //
                        futures::detail::meta::conditional_t<             //
                            forward_iterator<Rng>,       //
                            std::forward_iterator_tag,   //
                            std::input_iterator_tag>>>>>;

    /// \cond
    namespace ranges_detail {
        template <typename, bool> struct iterator_category_ {};

        template <typename I> struct iterator_category_<I, true> { using type = iterator_tag_of<I>; };

        template <typename T, typename U = futures::detail::meta::_t<std::remove_const<T>>>
        using iterator_category = iterator_category_<U, (bool)input_iterator<U>>;
    } // namespace ranges_detail
    /// \endcond

    /// \cond
    // Generally useful to know if an iterator is single-pass or not:
    // clang-format off
    template<typename I>
    CPP_concept single_pass_iterator_ =
        input_or_output_iterator<I> && !forward_iterator<I>;
    // clang-format on
    /// \endcond

    //////////////////////////////////////////////////////////////////////////////////////
    // indirect_result_t
    template <typename Fun, typename... Is>
    using indirect_result_t = ranges_detail::enable_if_t<(bool)and_v<(bool)indirectly_readable<Is>...>,
                                                  invoke_result_t<Fun, iter_reference_t<Is>...>>;

    /// \cond
    namespace ranges_detail {
        // clang-format off
        template(typename T1, typename T2, typename T3, typename T4)(
        concept (common_reference_with_4_impl_)(T1, T2, T3, T4),
            concepts::type<common_reference_t<T1, T2, T3, T4>>     AND
            convertible_to<T1, common_reference_t<T1, T2, T3, T4>> AND
            convertible_to<T2, common_reference_t<T1, T2, T3, T4>> AND
            convertible_to<T3, common_reference_t<T1, T2, T3, T4>> AND
            convertible_to<T4, common_reference_t<T1, T2, T3, T4>>
        );

        template<typename T1, typename T2, typename T3, typename T4>
        CPP_concept common_reference_with_4_ =
            CPP_concept_ref(ranges_detail::common_reference_with_4_impl_, T1, T2, T3, T4);
        // axiom: all permutations of T1,T2,T3,T4 have the same
        // common reference type.

        template(typename F, typename I)(
        concept (indirectly_unary_invocable_impl_)(F, I),
            invocable<F &, iter_value_t<I> &> AND
            invocable<F &, iter_reference_t<I>> AND
            invocable<F &, iter_common_reference_t<I>> AND
            common_reference_with<
                invoke_result_t<F &, iter_value_t<I> &>,
                invoke_result_t<F &, iter_reference_t<I>>>
        );

        template<typename F, typename I>
        CPP_concept indirectly_unary_invocable_ =
            indirectly_readable<I> &&
            CPP_concept_ref(ranges_detail::indirectly_unary_invocable_impl_, F, I);
        // clang-format on
    } // namespace ranges_detail
      /// \endcond

    // clang-format off
    template<typename F, typename I>
    CPP_concept indirectly_unary_invocable =
        ranges_detail::indirectly_unary_invocable_<F, I> &&
        copy_constructible<F>;

    template(typename F, typename I)(
    concept (indirectly_regular_unary_invocable_)(F, I),
        regular_invocable<F &, iter_value_t<I> &> AND
        regular_invocable<F &, iter_reference_t<I>> AND
        regular_invocable<F &, iter_common_reference_t<I>> AND
        common_reference_with<
            invoke_result_t<F &, iter_value_t<I> &>,
            invoke_result_t<F &, iter_reference_t<I>>>
    );

    template<typename F, typename I>
    CPP_concept indirectly_regular_unary_invocable =
        indirectly_readable<I> &&
        copy_constructible<F> &&
        CPP_concept_ref(::futures::detail::indirectly_regular_unary_invocable_, F, I);

    /// \cond
    // Non-standard indirect invocable concepts
    template(typename F, typename I1, typename I2)(
    concept (indirectly_binary_invocable_impl_)(F, I1, I2),
        invocable<F &, iter_value_t<I1> &, iter_value_t<I2> &> AND
        invocable<F &, iter_value_t<I1> &, iter_reference_t<I2>> AND
        invocable<F &, iter_reference_t<I1>, iter_value_t<I2> &> AND
        invocable<F &, iter_reference_t<I1>, iter_reference_t<I2>> AND
        invocable<F &, iter_common_reference_t<I1>, iter_common_reference_t<I2>> AND
        ranges_detail::common_reference_with_4_<
            invoke_result_t<F &, iter_value_t<I1> &, iter_value_t<I2> &>,
            invoke_result_t<F &, iter_value_t<I1> &, iter_reference_t<I2>>,
            invoke_result_t<F &, iter_reference_t<I1>, iter_value_t<I2> &>,
            invoke_result_t<F &, iter_reference_t<I1>, iter_reference_t<I2>>>
    );

    template<typename F, typename I1, typename I2>
    CPP_concept indirectly_binary_invocable_ =
        indirectly_readable<I1> && indirectly_readable<I2> &&
        copy_constructible<F> &&
        CPP_concept_ref(::futures::detail::indirectly_binary_invocable_impl_, F, I1, I2);

    template(typename F, typename I1, typename I2)(
    concept (indirectly_regular_binary_invocable_impl_)(F, I1, I2),
        regular_invocable<F &, iter_value_t<I1> &, iter_value_t<I2> &> AND
        regular_invocable<F &, iter_value_t<I1> &, iter_reference_t<I2>> AND
        regular_invocable<F &, iter_reference_t<I1>, iter_value_t<I2> &> AND
        regular_invocable<F &, iter_reference_t<I1>, iter_reference_t<I2>> AND
        regular_invocable<F &, iter_common_reference_t<I1>, iter_common_reference_t<I2>> AND
        ranges_detail::common_reference_with_4_<
            invoke_result_t<F &, iter_value_t<I1> &, iter_value_t<I2> &>,
            invoke_result_t<F &, iter_value_t<I1> &, iter_reference_t<I2>>,
            invoke_result_t<F &, iter_reference_t<I1>, iter_value_t<I2> &>,
            invoke_result_t<F &, iter_reference_t<I1>, iter_reference_t<I2>>>
    );

    template<typename F, typename I1, typename I2>
    CPP_concept indirectly_regular_binary_invocable_ =
        indirectly_readable<I1> && indirectly_readable<I2> &&
        copy_constructible<F> &&
        CPP_concept_ref(::futures::detail::indirectly_regular_binary_invocable_impl_, F, I1, I2);
    /// \endcond

    template(typename F, typename I)(
    concept (indirect_unary_predicate_)(F, I),
        predicate<F &, iter_value_t<I> &> AND
        predicate<F &, iter_reference_t<I>> AND
        predicate<F &, iter_common_reference_t<I>>
    );

    template<typename F, typename I>
    CPP_concept indirect_unary_predicate =
        indirectly_readable<I> &&
        copy_constructible<F> &&
        CPP_concept_ref(::futures::detail::indirect_unary_predicate_, F, I);

    template(typename F, typename I1, typename I2)(
    concept (indirect_binary_predicate_impl_)(F, I1, I2),
        predicate<F &, iter_value_t<I1> &, iter_value_t<I2> &> AND
        predicate<F &, iter_value_t<I1> &, iter_reference_t<I2>> AND
        predicate<F &, iter_reference_t<I1>, iter_value_t<I2> &> AND
        predicate<F &, iter_reference_t<I1>, iter_reference_t<I2>> AND
        predicate<F &, iter_common_reference_t<I1>, iter_common_reference_t<I2>>
    );

    template<typename F, typename I1, typename I2>
    CPP_concept indirect_binary_predicate_ =
        indirectly_readable<I1> && indirectly_readable<I2> &&
        copy_constructible<F> &&
        CPP_concept_ref(::futures::detail::indirect_binary_predicate_impl_, F, I1, I2);

    template(typename F, typename I1, typename I2)(
    concept (indirect_relation_)(F, I1, I2),
        relation<F &, iter_value_t<I1> &, iter_value_t<I2> &> AND
        relation<F &, iter_value_t<I1> &, iter_reference_t<I2>> AND
        relation<F &, iter_reference_t<I1>, iter_value_t<I2> &> AND
        relation<F &, iter_reference_t<I1>, iter_reference_t<I2>> AND
        relation<F &, iter_common_reference_t<I1>, iter_common_reference_t<I2>>
    );

    template<typename F, typename I1, typename I2 = I1>
    CPP_concept indirect_relation =
        indirectly_readable<I1> && indirectly_readable<I2> &&
        copy_constructible<F> &&
        CPP_concept_ref(::futures::detail::indirect_relation_, F, I1, I2);

    template(typename F, typename I1, typename I2)(
    concept (indirect_strict_weak_order_)(F, I1, I2),
        strict_weak_order<F &, iter_value_t<I1> &, iter_value_t<I2> &> AND
        strict_weak_order<F &, iter_value_t<I1> &, iter_reference_t<I2>> AND
        strict_weak_order<F &, iter_reference_t<I1>, iter_value_t<I2> &> AND
        strict_weak_order<F &, iter_reference_t<I1>, iter_reference_t<I2>> AND
        strict_weak_order<F &, iter_common_reference_t<I1>, iter_common_reference_t<I2>>
    );

    template<typename F, typename I1, typename I2 = I1>
    CPP_concept indirect_strict_weak_order =
        indirectly_readable<I1> && indirectly_readable<I2> &&
        copy_constructible<F> &&
        CPP_concept_ref(::futures::detail::indirect_strict_weak_order_, F, I1, I2);
    // clang-format on

    //////////////////////////////////////////////////////////////////////////////////////
    // projected struct, for "projecting" a readable with a unary callable
    /// \cond
    namespace ranges_detail {
        RANGES_DIAGNOSTIC_PUSH
        RANGES_DIAGNOSTIC_IGNORE_UNDEFINED_INTERNAL
        template <typename I, typename Proj> struct projected_ {
            struct type {
                using reference = indirect_result_t<Proj &, I>;
                using value_type = uncvref_t<reference>;
                reference operator*() const;
            };
        };
        RANGES_DIAGNOSTIC_POP

        template <typename Proj> struct select_projected_ {
            template <typename I>
            using apply = futures::detail::meta::_t<
                ranges_detail::enable_if_t<(bool)indirectly_regular_unary_invocable<Proj, I>, ranges_detail::projected_<I, Proj>>>;
        };

        template <> struct select_projected_<identity> {
            template <typename I> using apply = ranges_detail::enable_if_t<(bool)indirectly_readable<I>, I>;
        };
    } // namespace ranges_detail
    /// \endcond

    template <typename I, typename Proj> using projected = typename ranges_detail::select_projected_<Proj>::template apply<I>;

    template <typename I, typename Proj>
    struct incrementable_traits<ranges_detail::projected_<I, Proj>> : incrementable_traits<I> {};

    // clang-format off
    template(typename I, typename O)(
    concept (indirectly_movable_)(I, O),
        indirectly_writable<O, iter_rvalue_reference_t<I>>
    );

    template<typename I, typename O>
    CPP_concept indirectly_movable =
        indirectly_readable<I> && CPP_concept_ref(::futures::detail::indirectly_movable_, I, O);

    template(typename I, typename O)(
    concept (indirectly_movable_storable_)(I, O),
        indirectly_writable<O, iter_value_t<I>> AND
        movable<iter_value_t<I>> AND
        constructible_from<iter_value_t<I>, iter_rvalue_reference_t<I>> AND
        assignable_from<iter_value_t<I> &, iter_rvalue_reference_t<I>>
    );

    template<typename I, typename O>
    CPP_concept indirectly_movable_storable =
        indirectly_movable<I, O> &&
        CPP_concept_ref(::futures::detail::indirectly_movable_storable_, I, O);

    template(typename I, typename O)(
    concept (indirectly_copyable_)(I, O),
        indirectly_writable<O, iter_reference_t<I>>
    );

    template<typename I, typename O>
    CPP_concept indirectly_copyable =
        indirectly_readable<I> && CPP_concept_ref(::futures::detail::indirectly_copyable_, I, O);

    template(typename I, typename O)(
    concept (indirectly_copyable_storable_)(I, O),
        indirectly_writable<O, iter_value_t<I> const &> AND
        copyable<iter_value_t<I>> AND
        constructible_from<iter_value_t<I>, iter_reference_t<I>> AND
        assignable_from<iter_value_t<I> &, iter_reference_t<I>>
    );

    template<typename I, typename O>
    CPP_concept indirectly_copyable_storable =
        indirectly_copyable<I, O> &&
        CPP_concept_ref(::futures::detail::indirectly_copyable_storable_, I, O);

    template<typename I1, typename I2>
    CPP_requires(indirectly_swappable_,
        requires(I1 const i1, I2 const i2) //
        (
            ::futures::detail::iter_swap(i1, i2),
            ::futures::detail::iter_swap(i1, i1),
            ::futures::detail::iter_swap(i2, i2),
            ::futures::detail::iter_swap(i2, i1)
        ));
    template<typename I1, typename I2 = I1>
    CPP_concept indirectly_swappable =
        indirectly_readable<I1> && //
        indirectly_readable<I2> && //
        CPP_requires_ref(::futures::detail::indirectly_swappable_, I1, I2);

    template(typename C, typename I1, typename P1, typename I2, typename P2)(
    concept (projected_indirect_relation_)(C, I1, P1, I2, P2),
        indirect_relation<C, projected<I1, P1>, projected<I2, P2>>
    );

    template<typename I1, typename I2, typename C, typename P1 = identity,
        typename P2 = identity>
    CPP_concept indirectly_comparable =
        CPP_concept_ref(::futures::detail::projected_indirect_relation_, C, I1, P1, I2, P2);

    //////////////////////////////////////////////////////////////////////////////////////
    // Composite concepts for use defining algorithms:
    template<typename I>
    CPP_concept permutable =
        forward_iterator<I> &&
        indirectly_swappable<I, I> &&
        indirectly_movable_storable<I, I>;

    template(typename C, typename I1, typename P1, typename I2, typename P2)(
    concept (projected_indirect_strict_weak_order_)(C, I1, P1, I2, P2),
        indirect_strict_weak_order<C, projected<I1, P1>, projected<I2, P2>>
    );

    template<typename I1, typename I2, typename Out, typename C = less,
        typename P1 = identity, typename P2 = identity>
    CPP_concept mergeable =
        input_iterator<I1> &&
        input_iterator<I2> &&
        weakly_incrementable<Out> &&
        indirectly_copyable<I1, Out> &&
        indirectly_copyable<I2, Out> &&
        CPP_concept_ref(::futures::detail::projected_indirect_strict_weak_order_, C, I1, P1, I2, P2);

    template<typename I, typename C = less, typename P = identity>
    CPP_concept sortable =
        permutable<I> &&
        CPP_concept_ref(::futures::detail::projected_indirect_strict_weak_order_, C, I, P, I, P);
    // clang-format on

    struct sentinel_tag {};
    struct sized_sentinel_tag : sentinel_tag {};

    template <typename S, typename I>
    using sentinel_tag_of =               //
        std::enable_if_t<                 //
            sentinel_for<S, I>,           //
            futures::detail::meta::conditional_t<          //
                sized_sentinel_for<S, I>, //
                sized_sentinel_tag,       //
                sentinel_tag>>;

    // Deprecated things:
    /// \cond
    template <typename I>
    using iterator_category RANGES_DEPRECATED("iterator_category is deprecated. Use the iterator concepts instead") =
        ranges_detail::iterator_category<I>;

    template <typename I>
    using iterator_category_t
        RANGES_DEPRECATED("iterator_category_t is deprecated. Use the iterator concepts instead") =
            futures::detail::meta::_t<ranges_detail::iterator_category<I>>;

    template <typename Fun, typename... Is>
    using indirect_invoke_result_t
        RANGES_DEPRECATED("Please switch to indirect_result_t") = indirect_result_t<Fun, Is...>;

    template <typename Fun, typename... Is>
    struct RANGES_DEPRECATED("Please switch to indirect_result_t") indirect_invoke_result
        : futures::detail::meta::defer<indirect_result_t, Fun, Is...> {};

    template <typename Sig> struct indirect_result_of {};

    template <typename Fun, typename... Is>
    struct RANGES_DEPRECATED("Please switch to indirect_result_t") indirect_result_of<Fun(Is...)>
        : futures::detail::meta::defer<indirect_result_t, Fun, Is...> {};

    template <typename Sig>
    using indirect_result_of_t
        RANGES_DEPRECATED("Please switch to indirect_result_t") = futures::detail::meta::_t<indirect_result_of<Sig>>;
    /// \endcond

    namespace cpp20 {
        using ::futures::detail::bidirectional_iterator;
        using ::futures::detail::contiguous_iterator;
        using ::futures::detail::forward_iterator;
        using ::futures::detail::incrementable;
        using ::futures::detail::indirect_relation;
        using ::futures::detail::indirect_result_t;
        using ::futures::detail::indirect_strict_weak_order;
        using ::futures::detail::indirect_unary_predicate;
        using ::futures::detail::indirectly_comparable;
        using ::futures::detail::indirectly_copyable;
        using ::futures::detail::indirectly_copyable_storable;
        using ::futures::detail::indirectly_movable;
        using ::futures::detail::indirectly_movable_storable;
        using ::futures::detail::indirectly_readable;
        using ::futures::detail::indirectly_regular_unary_invocable;
        using ::futures::detail::indirectly_swappable;
        using ::futures::detail::indirectly_unary_invocable;
        using ::futures::detail::indirectly_writable;
        using ::futures::detail::input_iterator;
        using ::futures::detail::input_or_output_iterator;
        using ::futures::detail::mergeable;
        using ::futures::detail::output_iterator;
        using ::futures::detail::permutable;
        using ::futures::detail::projected;
        using ::futures::detail::random_access_iterator;
        using ::futures::detail::sentinel_for;
        using ::futures::detail::sized_sentinel_for;
        using ::futures::detail::sortable;
        using ::futures::detail::weakly_incrementable;
    } // namespace cpp20
    /// @}
} // namespace futures::detail

#ifdef _GLIBCXX_DEBUG
// HACKHACK: workaround underconstrained operator- for libstdc++ debug iterator wrapper
// by intentionally creating an ambiguity when the wrapped types don't support the
// necessary operation.
namespace __gnu_debug {
    template(typename I1, typename I2, typename Seq)(
        /// \pre
        requires(!::futures::detail::sized_sentinel_for<I1, I2>)) //
        void
        operator-(_Safe_iterator<I1, Seq> const &, _Safe_iterator<I2, Seq> const &) = delete;

    template(typename I1, typename Seq)(
        /// \pre
        requires(!::futures::detail::sized_sentinel_for<I1, I1>)) //
        void
        operator-(_Safe_iterator<I1, Seq> const &, _Safe_iterator<I1, Seq> const &) = delete;
} // namespace __gnu_debug
#endif

#if defined(__GLIBCXX__) || (defined(_LIBCPP_VERSION) && _LIBCPP_VERSION <= 3900)
// HACKHACK: workaround libc++ (https://llvm.org/bugs/show_bug.cgi?id=28421)
// and libstdc++ (https://gcc.gnu.org/bugzilla/show_bug.cgi?id=71771)
// underconstrained operator- for reverse_iterator by disabling sized_sentinel_for
// when the base iterators do not model sized_sentinel_for.

namespace futures::detail {
    template <typename S, typename I>
    /*inline*/ constexpr bool disable_sized_sentinel<std::reverse_iterator<S>, std::reverse_iterator<I>> =
        !static_cast<bool>(sized_sentinel_for<I, S>);
} // namespace futures::detail

#endif // defined(__GLIBCXX__) || (defined(_LIBCPP_VERSION) && _LIBCPP_VERSION <= 3900)

#include <futures/algorithm/detail/traits/range/detail/epilogue.h>

#endif // FUTURES_RANGES_ITERATOR_CONCEPTS_HPP
