/// \file
//  CPP, the Concepts PreProcessor library
//
//  Copyright Eric Niebler 2018-present
//  Copyright (c) 2020-present, Google LLC.
//
//  Use, modification and distribution is subject to the
//  Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
//
// This source code is licensed under the MIT license found in the
// LICENSE file in the root directory of this source tree.
//
// Project home: https://github.com/ericniebler/range-v3
//
#ifndef CPP_COMPARE_HPP
#define CPP_COMPARE_HPP

#if __cplusplus > 201703L && defined(__cpp_impl_three_way_comparison) && __has_include(<compare>)

#include <compare>
#include <futures/algorithm/detail/traits/range/compare.hpp>
#include <futures/algorithm/detail/traits/range/concepts/concepts.hpp>

// clang-format off

namespace futures::detail::concepts
{
    // Note: concepts in this file can use C++20 concepts, since operator<=> isn't available in
    // compilers that don't support core concepts.
    namespace ranges_detail
    {
        template<typename T, typename Cat>
        concept compares_as = same_as<futures::detail::common_comparison_category_t<T, Cat>, Cat>;
    } // namespace ranges_detail

    inline namespace defs
    {
        template<typename T, typename Cat = std::partial_ordering>
        concept three_way_comparable =
            ranges_detail::weakly_equality_comparable_with_<T, T> &&
            ranges_detail::partially_ordered_with_<T ,T> &&
            requires(ranges_detail::as_cref_t<T>& a, ranges_detail::as_cref_t<T>& b) {
                { a <=> b } -> ranges_detail::compares_as<Cat>;
            };

        template<typename T, typename U, typename Cat = std::partial_ordering>
        concept three_way_comparable_with =
            three_way_comparable<T, Cat> &&
            three_way_comparable<U, Cat> &&
            common_reference_with<ranges_detail::as_cref_t<T>&, ranges_detail::as_cref_t<U>&> &&
            three_way_comparable<common_reference_t<ranges_detail::as_cref_t<T>&, ranges_detail::as_cref_t<U>&>> &&
            ranges_detail::partially_ordered_with_<T, U> &&
            requires(ranges_detail::as_cref_t<T>& t, ranges_detail::as_cref_t<U>& u) {
                { t <=> u } -> ranges_detail::compares_as<Cat>;
                { u <=> t } -> ranges_detail::compares_as<Cat>;
            };
    } // inline namespace defs
} // namespace futures::detail::concepts

// clang-format on

#endif // __cplusplus
#endif // CPP_COMPARE_HPP
