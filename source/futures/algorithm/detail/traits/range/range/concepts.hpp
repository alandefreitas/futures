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

#ifndef FUTURES_RANGES_RANGE_CONCEPTS_HPP
#define FUTURES_RANGES_RANGE_CONCEPTS_HPP

#include <initializer_list>
#include <type_traits>
#include <utility>

#ifdef __has_include
#if __has_include(<span>)
#include <span>
#endif
#if __has_include(<string_view>)
#include <string_view>
#endif
#endif

#include <futures/algorithm/detail/traits/range/meta/meta.hpp>

#include <futures/algorithm/detail/traits/range/concepts/concepts.hpp>

#include <futures/algorithm/detail/traits/range/range_fwd.hpp>

#include <futures/algorithm/detail/traits/range/functional/comparisons.hpp>
#include <futures/algorithm/detail/traits/range/iterator/concepts.hpp>
#include <futures/algorithm/detail/traits/range/iterator/traits.hpp>
#include <futures/algorithm/detail/traits/range/range/access.hpp>
#include <futures/algorithm/detail/traits/range/range/primitives.hpp>
#include <futures/algorithm/detail/traits/range/range/traits.hpp>

#include <futures/algorithm/detail/traits/range/detail/prologue.hpp>

namespace futures::detail {
    /// \addtogroup group-range
    /// @{

    ///
    /// Range concepts below
    ///

    // clang-format off
    template<typename T>
    CPP_requires(_range_,
        requires(T & t) //
        (
            futures::detail::begin(t), // not necessarily equality-preserving
            futures::detail::end(t)
        ));
    template<typename T>
    CPP_concept range =
        CPP_requires_ref(futures::detail::_range_, T);

    template<typename T>
    CPP_concept borrowed_range =
        range<T> && ranges_detail::_borrowed_range<T>;

    template <typename R>
    RANGES_DEPRECATED("Please use futures::detail::borrowed_range instead.")
    RANGES_INLINE_VAR constexpr bool safe_range = borrowed_range<R>;

    template(typename T, typename V)(
    concept (output_range_)(T, V),
        output_iterator<iterator_t<T>, V>
    );
    template<typename T, typename V>
    CPP_concept output_range =
        range<T> && CPP_concept_ref(futures::detail::output_range_, T, V);

    template(typename T)(
    concept (input_range_)(T),
        input_iterator<iterator_t<T>>
    );
    template<typename T>
    CPP_concept input_range =
        range<T> && CPP_concept_ref(futures::detail::input_range_, T);

    template(typename T)(
    concept (forward_range_)(T),
        forward_iterator<iterator_t<T>>
    );
    template<typename T>
    CPP_concept forward_range =
        input_range<T> && CPP_concept_ref(futures::detail::forward_range_, T);

    template(typename T)(
    concept (bidirectional_range_)(T),
        bidirectional_iterator<iterator_t<T>>
    );
    template<typename T>
    CPP_concept bidirectional_range =
        forward_range<T> && CPP_concept_ref(futures::detail::bidirectional_range_, T);

    template(typename T)(
    concept (random_access_range_)(T),
        random_access_iterator<iterator_t<T>>
    );

    template<typename T>
    CPP_concept random_access_range =
        bidirectional_range<T> && CPP_concept_ref(futures::detail::random_access_range_, T);
    // clang-format on

    /// \cond
    namespace ranges_detail {
        template <typename Rng> using data_t = decltype(futures::detail::data(std::declval<Rng &>()));

        template <typename Rng> using element_t = futures::detail::meta::_t<std::remove_pointer<data_t<Rng>>>;
    } // namespace ranges_detail
      /// \endcond

    // clang-format off
    template(typename T)(
    concept (contiguous_range_)(T),
        contiguous_iterator<iterator_t<T>> AND
        same_as<ranges_detail::data_t<T>, std::add_pointer_t<iter_reference_t<iterator_t<T>>>>
    );

    template<typename T>
    CPP_concept contiguous_range =
        random_access_range<T> && CPP_concept_ref(futures::detail::contiguous_range_, T);

    template(typename T)(
    concept (common_range_)(T),
        same_as<iterator_t<T>, sentinel_t<T>>
    );

    template<typename T>
    CPP_concept common_range =
        range<T> && CPP_concept_ref(futures::detail::common_range_, T);

    /// \cond
    template<typename T>
    CPP_concept bounded_range =
        common_range<T>;
    /// \endcond

    template<typename T>
    CPP_requires(sized_range_,
        requires(T & t) //
        (
            futures::detail::size(t)
        ));
    template(typename T)(
    concept (sized_range_)(T),
        ranges_detail::integer_like_<range_size_t<T>>);

    template<typename T>
    CPP_concept sized_range =
        range<T> &&
        !disable_sized_range<uncvref_t<T>> &&
        CPP_requires_ref(futures::detail::sized_range_, T) &&
        CPP_concept_ref(futures::detail::sized_range_, T);
    // clang-format on

    /// \cond
    namespace ext {
        template <typename T> struct enable_view : std::is_base_of<view_base, T> {};
    } // namespace ext
    /// \endcond

    // Specialize this if the default is wrong.
    template <typename T> RANGES_INLINE_VAR constexpr bool enable_view = ext::enable_view<T>::value;

#if defined(__cpp_lib_string_view) && __cpp_lib_string_view > 0
    template <typename Char, typename Traits>
    RANGES_INLINE_VAR constexpr bool enable_view<std::basic_string_view<Char, Traits>> = true;
#endif

#if defined(__cpp_lib_span) && __cpp_lib_span > 0
    template <typename T, std::size_t N> RANGES_INLINE_VAR constexpr bool enable_view<std::span<T, N>> = N + 1 < 2;
#endif

    ///
    /// View concepts below
    ///

    // clang-format off
    template<typename T>
    CPP_concept view_ =
        range<T> &&
        semiregular<T> &&
        enable_view<T>;

    template<typename T>
    CPP_concept viewable_range =
        range<T> &&
        (borrowed_range<T> || view_<uncvref_t<T>>);
    // clang-format on

    //////////////////////////////////////////////////////////////////////////////////////
    // range_tag
    struct range_tag {};

    struct input_range_tag : range_tag {};
    struct forward_range_tag : input_range_tag {};
    struct bidirectional_range_tag : forward_range_tag {};
    struct random_access_range_tag : bidirectional_range_tag {};
    struct contiguous_range_tag : random_access_range_tag {};

    template <typename Rng>
    using range_tag_of =                          //
        std::enable_if_t<                         //
            range<Rng>,                           //
            futures::detail::meta::conditional_t<                  //
                contiguous_range<Rng>,            //
                contiguous_range_tag,             //
                futures::detail::meta::conditional_t<              //
                    random_access_range<Rng>,     //
                    random_access_range_tag,      //
                    futures::detail::meta::conditional_t<          //
                        bidirectional_range<Rng>, //
                        bidirectional_range_tag,  //
                        futures::detail::meta::conditional_t<      //
                            forward_range<Rng>,   //
                            forward_range_tag,    //
                            futures::detail::meta::conditional_t<  //
                                input_range<Rng>, //
                                input_range_tag,  //
                                range_tag>>>>>>;

    //////////////////////////////////////////////////////////////////////////////////////
    // common_range_tag_of
    struct common_range_tag : range_tag {};

    template <typename Rng>
    using common_range_tag_of = //
        std::enable_if_t<       //
            range<Rng>,         //
            futures::detail::meta::conditional_t<common_range<Rng>, common_range_tag, range_tag>>;

    //////////////////////////////////////////////////////////////////////////////////////
    // sized_range_concept
    struct sized_range_tag : range_tag {};

    template <typename Rng>
    using sized_range_tag_of = //
        std::enable_if_t<      //
            range<Rng>,        //
            futures::detail::meta::conditional_t<sized_range<Rng>, sized_range_tag, range_tag>>;

    /// \cond
    namespace view_detail_ {
        // clang-format off
        template<typename T>
        CPP_concept view =
            futures::detail::view_<T>;
        // clang-format on
    } // namespace view_detail_
    /// \endcond

    namespace cpp20 {
        using futures::detail::bidirectional_range;
        using futures::detail::borrowed_range;
        using futures::detail::common_range;
        using futures::detail::contiguous_range;
        using futures::detail::enable_view;
        using futures::detail::forward_range;
        using futures::detail::input_range;
        using futures::detail::output_range;
        using futures::detail::random_access_range;
        using futures::detail::range;
        using futures::detail::sized_range;
        using futures::detail::view_base;
        using futures::detail::viewable_range;
        using futures::detail::view_detail_::view;
    } // namespace cpp20
    /// @}
} // namespace futures::detail

#include <futures/algorithm/detail/traits/range/detail/epilogue.hpp>

#endif
