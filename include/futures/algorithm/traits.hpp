//
// Copyright (c) 2022 alandefreitas (alandefreitas@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//

#ifndef FUTURES_ALGORITHM_TRAITS_HPP
#define FUTURES_ALGORITHM_TRAITS_HPP

/**
 *  @dir algorithm/traits
 *  @brief Root traits directory
 *  @details This root directory contains all headers related to the algorithm
 *  traits module.
 */

/**
 *  @file algorithm/traits.hpp
 *  @brief All algorithm traits
 *
 *  Use this header to include all functionalities of the algorithm traits
 *  submodule at once. In most cases, however, we recommend only including the
 *  headers for the functionality you need. Use the reference to identify
 *  these files.
 */

// #glob <futures/algorithm/traits/*.hpp>
#include <futures/algorithm/traits/binary_invoke_algorithm.hpp>
#include <futures/algorithm/traits/is_assignable_from.hpp>
#include <futures/algorithm/traits/is_constructible_from.hpp>
#include <futures/algorithm/traits/is_convertible_to.hpp>
#include <futures/algorithm/traits/is_copyable.hpp>
#include <futures/algorithm/traits/is_default_initializable.hpp>
#include <futures/algorithm/traits/is_derived_from.hpp>
#include <futures/algorithm/traits/is_equality_comparable.hpp>
#include <futures/algorithm/traits/is_equality_comparable_with.hpp>
#include <futures/algorithm/traits/is_forward_iterator.hpp>
#include <futures/algorithm/traits/is_incrementable.hpp>
#include <futures/algorithm/traits/is_indirectly_binary_invocable.hpp>
#include <futures/algorithm/traits/is_indirectly_readable.hpp>
#include <futures/algorithm/traits/is_indirectly_unary_invocable.hpp>
#include <futures/algorithm/traits/is_input_iterator.hpp>
#include <futures/algorithm/traits/is_input_or_output_iterator.hpp>
#include <futures/algorithm/traits/is_input_range.hpp>
#include <futures/algorithm/traits/is_movable.hpp>
#include <futures/algorithm/traits/is_move_constructible.hpp>
#include <futures/algorithm/traits/is_partially_ordered_with.hpp>
#include <futures/algorithm/traits/is_range.hpp>
#include <futures/algorithm/traits/is_regular.hpp>
#include <futures/algorithm/traits/is_semiregular.hpp>
#include <futures/algorithm/traits/is_sentinel_for.hpp>
#include <futures/algorithm/traits/is_swappable.hpp>
#include <futures/algorithm/traits/is_totally_ordered.hpp>
#include <futures/algorithm/traits/is_totally_ordered_with.hpp>
#include <futures/algorithm/traits/is_weakly_equality_comparable.hpp>
#include <futures/algorithm/traits/is_weakly_incrementable.hpp>
#include <futures/algorithm/traits/iter_difference.hpp>
#include <futures/algorithm/traits/iter_reference.hpp>
#include <futures/algorithm/traits/iter_rvalue_reference.hpp>
#include <futures/algorithm/traits/iter_value.hpp>
#include <futures/algorithm/traits/iterator.hpp>
#include <futures/algorithm/traits/range_value.hpp>
#include <futures/algorithm/traits/remove_cvref.hpp>
#include <futures/algorithm/traits/unary_invoke_algorithm.hpp>
#include <futures/algorithm/traits/value_cmp_algorithm.hpp>

#endif // FUTURES_ALGORITHM_TRAITS_HPP
