//
// Copyright (c) 2021 alandefreitas (alandefreitas@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//

#ifndef FUTURES_FUTURES_HPP
#define FUTURES_FUTURES_HPP

/**
 *  @dir futures
 *  @brief Root library directory
 *  @details The root directory contains headers related to the main module,
 *  including futures and basic functionality.
 */

/**
 *  @file futures.hpp
 *  @brief All functionality in the library
 *  @details Use this header to include all the library functionalities at
 *  once. In most cases, however, we recommend only including the headers for
 * the functionality you need. Use the reference to identify these files.
 */

#include <futures/config.hpp>

// #glob <futures/*.hpp> - <futures/config.hpp>
#include <futures/adaptor.hpp>
#include <futures/algorithm.hpp>
#include <futures/await.hpp>
#include <futures/error.hpp>
#include <futures/executor.hpp>
#include <futures/future.hpp>
#include <futures/future_options.hpp>
#include <futures/future_options_args.hpp>
#include <futures/future_status.hpp>
#include <futures/is_ready.hpp>
#include <futures/launch.hpp>
#include <futures/packaged_task.hpp>
#include <futures/promise.hpp>
#include <futures/stop_token.hpp>
#include <futures/throw.hpp>
#include <futures/traits.hpp>
#include <futures/wait_for_all.hpp>
#include <futures/wait_for_any.hpp>


#ifdef FUTURES_DOXYGEN
/// Main library namespace
namespace futures {}
#endif

#endif // FUTURES_FUTURES_HPP
