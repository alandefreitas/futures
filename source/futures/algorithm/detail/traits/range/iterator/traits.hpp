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

#ifndef FUTURES_RANGES_ITERATOR_TRAITS_HPP
#define FUTURES_RANGES_ITERATOR_TRAITS_HPP

#include <iterator>
#include <type_traits>

#include <futures/algorithm/detail/traits/range/meta/meta.hpp>

#include <futures/algorithm/detail/traits/range/concepts/concepts.hpp>

#include <futures/algorithm/detail/traits/range/range_fwd.hpp>

#include <futures/algorithm/detail/traits/range/iterator/access.hpp> // for iter_move, iter_swap
#include <futures/algorithm/detail/traits/range/utility/common_type.hpp>

#include <futures/algorithm/detail/traits/range/detail/prologue.hpp>

namespace futures::detail {
    /// \addtogroup group-iterator
    /// @{

    /// \cond
    using input_iterator_tag RANGES_DEPRECATED("Please switch to the standard iterator tags") = std::input_iterator_tag;
    using forward_iterator_tag
        RANGES_DEPRECATED("Please switch to the standard iterator tags") = std::forward_iterator_tag;
    using bidirectional_iterator_tag
        RANGES_DEPRECATED("Please switch to the standard iterator tags") = std::bidirectional_iterator_tag;
    using random_access_iterator_tag
        RANGES_DEPRECATED("Please switch to the standard iterator tags") = std::random_access_iterator_tag;
    /// \endcond

    struct contiguous_iterator_tag : std::random_access_iterator_tag {};

    /// \cond
    namespace ranges_detail {
        template <typename I, typename = iter_reference_t<I>, typename R = decltype(iter_move(std::declval<I &>())),
                  typename = R &>
        using iter_rvalue_reference_t = R;

        template <typename I>
        RANGES_INLINE_VAR constexpr bool
            has_nothrow_iter_move_v = noexcept(iter_rvalue_reference_t<I>(::futures::detail::iter_move(std::declval<I &>())));
    } // namespace ranges_detail
    /// \endcond

    template <typename I> using iter_rvalue_reference_t = ranges_detail::iter_rvalue_reference_t<I>;

    template <typename I> using iter_common_reference_t = common_reference_t<iter_reference_t<I>, iter_value_t<I> &>;

#if defined(RANGES_DEEP_STL_INTEGRATION) && RANGES_DEEP_STL_INTEGRATION && !defined(RANGES_DOXYGEN_INVOKED)
    template <typename T>
    using iter_difference_t = typename futures::detail::meta::conditional_t<ranges_detail::is_std_iterator_traits_specialized_v<T>,
                                                           std::iterator_traits<uncvref_t<T>>,
                                                           incrementable_traits<uncvref_t<T>>>::difference_type;
#else
    template <typename T> using iter_difference_t = typename incrementable_traits<uncvref_t<T>>::difference_type;
#endif

    // Defined in <range/v3/iterator/access.hpp>
    // template<typename T>
    // using iter_value_t = ...

    // Defined in <range/v3/iterator/access.hpp>
    // template<typename R>
    // using iter_reference_t = ranges_detail::iter_reference_t_<R>;

    // Defined in <range/v3/range_fwd.hpp>:
    // template<typename S, typename I>
    // inline constexpr bool disable_sized_sentinel = false;

    /// \cond
    namespace ranges_detail {
        template <typename I>
        using iter_size_t =
            futures::detail::meta::_t<futures::detail::meta::conditional_t<std::is_integral<iter_difference_t<I>>::value,
                                         std::make_unsigned<iter_difference_t<I>>, futures::detail::meta::id<iter_difference_t<I>>>>;

        template <typename I> using iter_arrow_t = decltype(std::declval<I &>().operator->());

        template <typename I>
        using iter_pointer_t =
            futures::detail::meta::_t<futures::detail::meta::conditional_t<futures::detail::meta::is_trait<futures::detail::meta::defer<iter_arrow_t, I>>::value,
                                         futures::detail::meta::defer<iter_arrow_t, I>, std::add_pointer<iter_reference_t<I>>>>;

        template <typename T> struct difference_type_ : futures::detail::meta::defer<iter_difference_t, T> {};

        template <typename T> struct value_type_ : futures::detail::meta::defer<iter_value_t, T> {};

        template <typename T> struct size_type_ : futures::detail::meta::defer<iter_size_t, T> {};
    } // namespace ranges_detail

    template <typename T>
    using difference_type_t RANGES_DEPRECATED("futures::detail::difference_type_t is deprecated. Please use "
                                              "futures::detail::iter_difference_t instead.") = iter_difference_t<T>;

    template <typename T>
    using value_type_t RANGES_DEPRECATED("futures::detail::value_type_t is deprecated. Please use "
                                         "futures::detail::iter_value_t instead.") = iter_value_t<T>;

    template <typename R>
    using reference_t RANGES_DEPRECATED("futures::detail::reference_t is deprecated. Use futures::detail::iter_reference_t "
                                        "instead.") = iter_reference_t<R>;

    template <typename I>
    using rvalue_reference_t RANGES_DEPRECATED("rvalue_reference_t is deprecated; "
                                               "use iter_rvalue_reference_t instead") = iter_rvalue_reference_t<I>;

    template <typename T>
    struct RANGES_DEPRECATED("futures::detail::size_type is deprecated. Iterators do not have an associated "
                             "size_type.") size_type : ranges_detail::size_type_<T> {};

    template <typename I> using size_type_t RANGES_DEPRECATED("size_type_t is deprecated.") = ranges_detail::iter_size_t<I>;
    /// \endcond

    namespace cpp20 {
        using ::futures::detail::iter_common_reference_t;
        using ::futures::detail::iter_difference_t;
        using ::futures::detail::iter_reference_t;
        using ::futures::detail::iter_rvalue_reference_t;
        using ::futures::detail::iter_value_t;

        // Specialize these in the ::futures::detail:: namespace
        using ::futures::detail::disable_sized_sentinel;
        template <typename T> using incrementable_traits = ::futures::detail::incrementable_traits<T>;
        template <typename T> using indirectly_readable_traits = ::futures::detail::indirectly_readable_traits<T>;
    } // namespace cpp20
    /// @}
} // namespace futures::detail

#include <futures/algorithm/detail/traits/range/detail/epilogue.hpp>

#endif // FUTURES_RANGES_ITERATOR_TRAITS_HPP
