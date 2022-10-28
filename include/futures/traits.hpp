//
// Copyright (c) 2021 alandefreitas (alandefreitas@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//

#ifndef FUTURES_TRAITS_HPP
#define FUTURES_TRAITS_HPP

/**
 *  @dir futures/traits
 *  @brief Root traits directory
 *  @details This directory contains headers related to the future traits
 *  module.
 */

/**
 *  @file traits.hpp
 *  @brief All Futures Traits
 *
 *  Use this header to include all functionalities of the futures traits module
 *  at once. In most cases, however, we recommend only including the headers for
 *  the functionality you need. Use the reference to identify these files.
 */

// #glob <futures/traits/*.hpp>
#include <futures/traits/future_value.hpp>
#include <futures/traits/has_executor.hpp>
#include <futures/traits/has_ready_notifier.hpp>
#include <futures/traits/has_stop_token.hpp>
#include <futures/traits/is_always_deferred.hpp>
#include <futures/traits/is_continuable.hpp>
#include <futures/traits/is_future.hpp>
#include <futures/traits/is_shared_future.hpp>
#include <futures/traits/is_stoppable.hpp>


#endif // FUTURES_TRAITS_HPP
