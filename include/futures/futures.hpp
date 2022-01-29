//
// Copyright (c) 2021 alandefreitas (alandefreitas@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//

#ifndef FUTURES_FUTURES_H
#define FUTURES_FUTURES_H

/// \file
/// Future types and functions to work with futures
///
/// Many of the ideas for these functions are based on:
/// - extensions for concurrency (ISO/IEC TS 19571:2016)
/// - async++
/// - continuable
/// - TBB
///
/// However, we use std::future and the ASIO proposed standard executors
/// (P0443r13, P1348r0, and P1393r0) to allow for better interoperability with
/// the C++ standard.
/// - the async function can accept any standard executor
/// - the async function will use a reasonable default thread pool when no
/// executor is provided
/// - future-concepts allows for new future classes to extend functionality
/// while reusing algorithms
/// - a cancellable future class is provided for more sensitive use cases
/// - the API can be updated as the standard gets updated
/// - the standard algorithms are reimplemented with a preference for parallel
/// operations
///
/// This interoperability comes at a price for continuations, as we might need
/// to poll for when_all/when_any/then events, because std::future does not have
/// internal continuations.
///
/// Although we attempt to replicate these features without recreating the
/// future class with internal continuations, we use a number of heuristics to
/// avoid polling for when_all/when_any/then:
/// - we allow for other future-like classes to be implemented through a
/// future-concept and provide these
///   functionalities at a lower cost whenever we can
/// - `when_all` (or operator&&) returns a when_all_future class, which does not
/// create a new std::future at all
///    and can check directly if futures are ready
/// - `when_any` (or operator||) returns a when_any_future class, which
/// implements a number of heuristics to avoid
///    polling, limit polling time, increased pooling intervals, and only
///    launching the necessary continuation futures for long tasks. (although
///    when_all always takes longer than when_any, when_any involves a number of
///    heuristics that influence its performance)
/// - `then` (or operator>>) returns a new future object that sleeps while the
/// previous future isn't ready
/// - when the standard supports that, this approach based on concepts also
/// serve as extension points to allow
///   for these proxy classes to change their behavior to some other algorithm
///   that makes more sense for futures that support continuations,
///   cancellation, progress, queries, .... More interestingly, the concepts
///   allow for all these possible future types to interoperate.
///
/// \see https://en.cppreference.com/w/cpp/experimental/concurrency
/// \see https://think-async.com/Asio/asio-1.18.2/doc/asio/std_executors.html
/// \see https://github.com/Amanieu/asyncplusplus

// Future classes
#include <futures/futures/async.hpp>
#include <futures/futures/await.hpp>
#include <futures/futures/basic_future.hpp>
#include <futures/futures/packaged_task.hpp>
#include <futures/futures/promise.hpp>
#include <futures/futures/wait_for_all.hpp>
#include <futures/futures/wait_for_any.hpp>

// Adaptors
#include <futures/adaptor/ready_future.hpp>
#include <futures/adaptor/then.hpp>
#include <futures/adaptor/when_all.hpp>
#include <futures/adaptor/when_any.hpp>

#endif // FUTURES_FUTURES_H
