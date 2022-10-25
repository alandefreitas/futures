//
// Copyright (c) 2021 alandefreitas (alandefreitas@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//

#ifndef FUTURES_ALGORITHM_HPP
#define FUTURES_ALGORITHM_HPP

/**
 *  @dir algorithm
 *  @brief Root algorithms directory
 *  @details The root directory contains headers related to the algorithm module,
 *  including algorithms, partitioners, and traits.
 */

/**
 *  @file algorithm.hpp
 *  @brief All Algorithms
 *
 *  Use this header to include all functionalities of the adaptors module at
 *  once. In most cases, however, we recommend only including the headers for the
 *  functionality you need. Use the reference to identify these files.
 */

// #glob <futures/algorithm/*.hpp>
#include <futures/algorithm/all_of.hpp>
#include <futures/algorithm/any_of.hpp>
#include <futures/algorithm/count.hpp>
#include <futures/algorithm/count_if.hpp>
#include <futures/algorithm/find.hpp>
#include <futures/algorithm/find_if.hpp>
#include <futures/algorithm/find_if_not.hpp>
#include <futures/algorithm/for_each.hpp>
#include <futures/algorithm/none_of.hpp>
#include <futures/algorithm/reduce.hpp>

#endif // FUTURES_ALGORITHM_HPP
