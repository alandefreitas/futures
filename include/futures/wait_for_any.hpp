//
// Copyright (c) 2021 alandefreitas (alandefreitas@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//

#ifndef FUTURES_WAIT_FOR_ANY_HPP
#define FUTURES_WAIT_FOR_ANY_HPP

/**
 *  @file wait_for_any.hpp
 *  @brief Functions to wait for any futures in a sequence
 *
 *  This file defines functions to wait for any future in a sequence of
 *  futures.
 */

#include <futures/wait_for_all.hpp>
#include <futures/algorithm/traits/is_range.hpp>
#include <futures/algorithm/traits/iter_value.hpp>
#include <futures/algorithm/traits/range_value.hpp>
#include <futures/traits/is_future.hpp>
#include <futures/detail/traits/std_type_traits.hpp>
#include <futures/detail/waiter_for_any.hpp>
#include <type_traits>

namespace futures {
    /** @addtogroup futures Futures
     *  @{
     */
    /** @addtogroup waiting Waiting
     *  @{
     */

    /// Wait for any future in a sequence to be ready
    /**
     *  This function waits for any future in the range [`first`, `last`) to be
     *  ready.
     *
     *  Unlike @ref wait_for_all, this function requires special data structures
     *  to allow that to happen without blocking.
     *
     *  For disjunctions, we have few options:
     *  - If the input futures support external notifiers:
     *      - Attach continuations to notify when a task is over
     *  - If the input futures do not have lazy continuations:
     *      - Polling in a busy loop until one of the futures is ready
     *      - Polling with exponential backoffs until one of the futures is
     *      ready
     *      - Launching n continuation tasks that set a promise when one of the
     *    futures is ready
     *      - Hybrids, usually polling for short tasks and launching threads for
     *    other tasks
     *  - If the input futures are mixed in regards to lazy continuations:
     *      - Mix the strategies above, depending on each input future
     *
     *  If the thresholds for these strategies are reasonable, this should be
     *  efficient for futures with or without lazy continuations.
     *
     *  @note This function is adapted from `boost::wait_for_any`
     *
     *  @see
     *  [boost.thread
     *  wait_for_any](https://www.boost.org/doc/libs/1_78_0/doc/html/thread/synchronization.html#thread.synchronization.futures.reference.wait_for_any)
     *
     *  @tparam Iterator Iterator type in a range of futures
     *  @param first Iterator to the first element in the range
     *  @param last Iterator to one past the last element in the range
     *  @return Iterator to the first future that got ready
     */
    FUTURES_TEMPLATE(class Iterator)
    (requires is_future_v<iter_value_t<Iterator>>) Iterator
        wait_for_any(Iterator first, Iterator last);

    /// Wait for any future in a sequence to be ready
    /**
     *  This function waits for any future in the range `r` to be ready.
     *  This function requires special data structures to allow that to happen
     *  without blocking.
     *
     *  @note This function is adapted from `boost::wait_for_any`
     *
     *  @see
     *  [boost.thread
     *  wait_for_any](https://www.boost.org/doc/libs/1_78_0/doc/html/thread/synchronization.html#thread.synchronization.futures.reference.wait_for_any)
     *
     *  @tparam Iterator A range of futures type
     *  @param r Range of futures
     *  @return Iterator to the first future that got ready
     */
    FUTURES_TEMPLATE(class Range)
    (requires is_range_v<Range> &&is_future_v<range_value_t<Range>>)
        iterator_t<Range> wait_for_any(Range &&r) {
        return wait_for_any(std::begin(r), std::end(r));
    }

    /// Wait for any future in a sequence to be ready
    /**
     *  This function waits for all specified futures `fs`... to be ready.
     *
     *  @note This function is adapted from `boost::wait_for_any`
     *
     *  @see
     *  [boost.thread
     *  wait_for_any](https://www.boost.org/doc/libs/1_78_0/doc/html/thread/synchronization.html#thread.synchronization.futures.reference.wait_for_any)
     *
     *  @tparam Fs A list of future types
     *  @param fs A list of future objects
     *  @return Index of the first future that got ready
     */
    FUTURES_TEMPLATE(class... Fs)
    (requires detail::conjunction_v<is_future<std::decay_t<Fs>>...>) std::size_t
        wait_for_any(Fs &&...fs);

    /// Wait for any future in a tuple to be ready
    /**
     *  This function waits for all specified futures `fs`... to be ready.
     *
     *  @note This function is adapted from `boost::wait_for_any`
     *
     *  @see
     *  [boost.thread
     *  wait_for_any](https://www.boost.org/doc/libs/1_78_0/doc/html/thread/synchronization.html#thread.synchronization.futures.reference.wait_for_any)
     *
     *  @tparam Fs A list of future types
     *
     *  @param t A list of future objects
     *
     *  @return Index of the first future that got ready
     */
    FUTURES_TEMPLATE(class Tuple)
    (requires detail::mp_similar<std::tuple<>, std::decay_t<Tuple>>::value)
        std::size_t wait_for_any(Tuple &&t);

    /// Wait for any future in a sequence to be ready
    /**
     *  @tparam Iterator Iterator type in a range of futures
     *  @tparam Rep Duration Rep
     *  @tparam Period Duration Period
     *  @param timeout_duration Time to wait for
     *  @param first Iterator to the first element in the range
     *  @param last Iterator to one past the last element in the range
     *
     *  @return Iterator to the future which got ready
     */
    FUTURES_TEMPLATE(typename Iterator, class Rep, class Period)
    (requires is_future_v<iter_value_t<Iterator>>) Iterator wait_for_any_for(
        std::chrono::duration<Rep, Period> const &timeout_duration,
        Iterator first,
        Iterator last);

    /// Wait for any future in a sequence to be ready
    /**
     *  @tparam Range Iterator type in a range of futures
     *  @tparam Rep Duration Rep
     *  @tparam Period Duration Period
     *  @param timeout_duration Time to wait for
     *  @param r Range of futures
     *
     *  @return Iterator to the future which got ready
     */
    FUTURES_TEMPLATE(typename Range, class Rep, class Period)
    (requires is_range_v<Range> &&is_future_v<range_value_t<Range>>)
        iterator_t<Range> wait_for_any_for(
            std::chrono::duration<Rep, Period> const &timeout_duration,
            Range &&r) {
        return wait_for_any_for(timeout_duration, std::begin(r), std::end(r));
    }


    /// Wait for any future in a sequence to be ready
    /**
     *  @tparam Fs Future types
     *  @tparam Rep Duration Rep
     *  @tparam Period Duration Period
     *  @param timeout_duration Time to wait for
     *  @param fs Future objects
     *
     *  @return Index of the future which got ready
     */
    FUTURES_TEMPLATE(typename... Fs, class Rep, class Period)
    (requires detail::conjunction_v<is_future<std::decay_t<Fs>>...>) std::size_t
        wait_for_any_for(
            std::chrono::duration<Rep, Period> const &timeout_duration,
            Fs &&...fs);

    /// Wait for any future in a sequence to be ready
    /**
     *  @tparam Tuple Tuple of futures
     *  @tparam Rep Duration Rep
     *  @tparam Period Duration Period
     *  @param timeout_duration Time to wait for
     *  @param t tuple of futures
     *
     *  @return Index of the future which got ready
     */
    FUTURES_TEMPLATE(class Tuple, class Rep, class Period)
    (requires detail::mp_similar<std::tuple<>, std::decay_t<Tuple>>::value)
        std::size_t wait_for_any_for(
            std::chrono::duration<Rep, Period> const &timeout_duration,
            Tuple &&t);

    /// Wait for any future in a sequence to be ready
    /**
     *  @tparam Iterator Iterator type in a range of futures
     *  @tparam Clock Time point clock
     *  @tparam Duration Time point duration
     *  @param timeout_time Limit time point
     *  @param first Iterator to the first element in the range
     *  @param last Iterator to one past the last element in the range
     *
     *  @return Iterator to the future which got ready
     */
    FUTURES_TEMPLATE(typename Iterator, class Clock, class Duration)
    (requires is_future_v<iter_value_t<Iterator>>) Iterator wait_for_any_until(
        std::chrono::time_point<Clock, Duration> const &timeout_time,
        Iterator first,
        Iterator last);

    /// Wait for any future in a sequence to be ready
    /**
     *  @tparam Range Range of futures
     *  @tparam Clock Time point clock
     *  @tparam Duration Time point duration
     *  @param timeout_time Limit time point
     *  @param r Range of futures
     *
     *  @return Iterator to the future which got ready
     */
    FUTURES_TEMPLATE(typename Range, class Clock, class Duration)
    (requires is_range_v<Range> &&is_future_v<range_value_t<Range>>)
        iterator_t<Range> wait_for_any_until(
            std::chrono::time_point<Clock, Duration> const &timeout_time,
            Range &&r) {
        return wait_for_any_until(timeout_time, std::begin(r), std::end(r));
    }


    /// Wait for any future in a sequence to be ready
    /**
     *  @tparam Fs Future types
     *  @tparam Clock Time point clock
     *  @tparam Duration Time point duration
     *  @param timeout_time Limit time point
     *  @param fs Future objects
     *
     *  @return Index of the future which got ready
     */
    FUTURES_TEMPLATE(typename... Fs, class Clock, class Duration)
    (requires detail::conjunction_v<is_future<std::decay_t<Fs>>...>) std::size_t
        wait_for_any_until(
            std::chrono::time_point<Clock, Duration> const &timeout_time,
            Fs &&...fs);

    /// Wait for any future in a sequence to be ready
    /**
     *  @tparam Tuple Tuple of future types
     *  @tparam Clock Time point clock
     *  @tparam Duration Time point duration
     *  @param timeout_time Limit time point
     *  @param t Tuple of future objects
     *
     *  @return Index of the future which got ready
     */
    FUTURES_TEMPLATE(class Tuple, class Clock, class Duration)
    (requires detail::mp_similar<std::tuple<>, std::decay_t<Tuple>>::value)
        std::size_t wait_for_any_until(
            std::chrono::time_point<Clock, Duration> const &timeout_time,
            Tuple &&t);

    /** @} */
    /** @} */
} // namespace futures

#include <futures/impl/wait_for_any.hpp>

#endif // FUTURES_WAIT_FOR_ANY_HPP
