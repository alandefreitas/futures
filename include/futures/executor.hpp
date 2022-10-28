//
// Copyright (c) 2021 alandefreitas (alandefreitas@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//

#ifndef FUTURES_EXECUTOR_HPP
#define FUTURES_EXECUTOR_HPP

/**
 *  @dir executor
 *  @brief Root executors directory
 *  @details The root directory contains headers related to the executor module,
 *  including executors and traits.
 */

/**
 *  @file executor.hpp
 *  @brief All Executors
 *
 *  Use this header to include all functionalities of the executors module at
 *  once. In most cases, however, we recommend only including the headers for the
 *  functionality you need. Use the reference to identify these files.
 */

// #glob <futures/executor/*.hpp>
#include <futures/executor/default_executor.hpp>
#include <futures/executor/hardware_concurrency.hpp>
#include <futures/executor/inline_executor.hpp>
#include <futures/executor/is_executor.hpp>
#include <futures/executor/new_thread_executor.hpp>


#endif // FUTURES_EXECUTOR_HPP
