//
// Copyright (c) 2021 alandefreitas (alandefreitas@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//

#ifndef FUTURES_ADAPTOR_HPP
#define FUTURES_ADAPTOR_HPP

/**
 *  @dir adaptor
 *  @brief Root adaptors directory
 *  @details The root directory contains headers related to the adaptor module,
 *  including conjunctions, disjunctions, and continuations.
 */

/**
 *  @file adaptor.hpp
 *  @brief All Adaptors
 *
 *  Use this header to include all functionalities of the algorithms module at
 *  once. In most cases, however, we recommend only including the headers for the
 *  functionality you need. Use the reference to identify these files.
 */


// #glob <futures/adaptor/*.hpp>
#include <futures/adaptor/bind_executor_to_lambda.hpp>
#include <futures/adaptor/make_ready_future.hpp>
#include <futures/adaptor/then.hpp>
#include <futures/adaptor/when_all.hpp>
#include <futures/adaptor/when_any.hpp>


#endif // FUTURES_ADAPTOR_HPP
