//
// Copyright (c) 2021 alandefreitas (alandefreitas@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//

#ifndef FUTURES_WAIT_FOR_ALL_HPP
#define FUTURES_WAIT_FOR_ALL_HPP

/**
 *  @file wait_for_all.hpp
 *  @brief Functions to wait for all futures in a sequence
 *
 *  This file defines functions to wait for all futures in a sequence of
 *  futures.
 */

#include <futures/adaptor/make_ready_future.hpp>
#include <futures/algorithm/traits/is_range.hpp>
#include <futures/algorithm/traits/iter_value.hpp>
#include <futures/algorithm/traits/range_value.hpp>
#include <futures/traits/is_future.hpp>
#include <tuple>
#include <type_traits>

namespace futures {
    /** @addtogroup futures Futures
     *  @{
     */
    /** @addtogroup waiting Waiting
     *  @{
     */

    /// Wait for a sequence of futures to be ready
    /**
     *  This function waits for all futures in the range [`first`, `last`) to be
     *  ready. It simply waits iteratively for each of the futures to be ready.
     *
     *  @note This function is adapted from boost::wait_for_all
     *
     *  @see
     *  [boost.thread
     *  wait_for_all](https://www.boost.org/doc/libs/1_78_0/doc/html/thread/synchronization.html#thread.synchronization.futures.reference.wait_for_all)
     *
     *  @tparam Iterator Iterator type in a range of futures
     *  @param first Iterator to the first element in the range
     *  @param last Iterator to one past the last element in the range
     */
    FUTURES_TEMPLATE(class Iterator)
    (requires is_future_v<iter_value_t<Iterator>>) void wait_for_all(
        Iterator first,
        Iterator last) {
        for (Iterator it = first; it != last; ++it) {
            it->wait();
        }
    }

    /// Wait for a sequence of futures to be ready
    /**
     *  This function waits for all futures in the range `r` to be ready.
     *  It simply waits iteratively for each of the futures to be ready.
     *
     *  @note This function is adapted from boost::wait_for_all
     *
     *  @see
     *  [boost.thread
     *  wait_for_all](https://www.boost.org/doc/libs/1_78_0/doc/html/thread/synchronization.html#thread.synchronization.futures.reference.wait_for_all)
     *
     *  @tparam Range A range of futures type
     *  @param r Range of futures
     */
    FUTURES_TEMPLATE(class Range)
    (requires is_range_v<Range>
         &&is_future_v<range_value_t<Range>>) void wait_for_all(Range &&r) {
        using std::begin;
        wait_for_all(begin(r), end(r));
    }

    /// Wait for a sequence of futures to be ready
    /**
     *  This function waits for all specified futures `fs`... to be ready.
     *
     *  It creates a compile-time fixed-size data structure to store references
     *  to all of the futures and then waits for each of the futures to be
     *  ready.
     *
     *  @note This function is adapted from boost::wait_for_all
     *
     *  @see
     *  [boost.thread
     *  wait_for_all](https://www.boost.org/doc/libs/1_78_0/doc/html/thread/synchronization.html#thread.synchronization.futures.reference.wait_for_all)
     *
     *  @tparam Fs A list of future types
     *  @param fs A list of future objects
     */
    FUTURES_TEMPLATE(class... Fs)
    (requires std::conjunction_v<
        is_future<std::decay_t<Fs>>...>) void wait_for_all(Fs &&...fs) {
        (fs.wait(), ...);
    }

    /// Wait for a sequence of futures to be ready
    FUTURES_TEMPLATE(class Tuple)
    (requires(detail::mp_similar<std::tuple<>, std::decay_t<Tuple>>::
                  value)) void wait_for_all(Tuple &&t) {
        tuple_for_each(std::forward<Tuple>(t), [](auto &f) { f.wait(); });
    }

    /// Wait for a sequence of futures to be ready
    /**
     *  @tparam Iterator Iterator type in a range of futures
     *  @tparam Rep Duration Rep
     *  @tparam Period Duration Period
     *  @param timeout_duration Time to wait for
     *  @param first Iterator to the first element in the range
     *  @param last Iterator to one past the last element in the range
     *
     *  @return `future_status::ready` if all futures got ready.
     *  `future_status::timeout` otherwise.
     */
    FUTURES_TEMPLATE(class Iterator, class Rep, class Period)
    (requires is_future_v<iter_value_t<Iterator>>) future_status
        wait_for_all_for(
            std::chrono::duration<Rep, Period> const &timeout_duration,
            Iterator first,
            Iterator last);

    /// Wait for a sequence of futures to be ready
    /**
     *  @tparam Range Range of futures
     *  @tparam Rep Duration Rep
     *  @tparam Period Duration Period
     *  @param timeout_duration Time to wait for
     *  @param r Range of futures
     *
     *  @return `future_status::ready` if all futures got ready.
     *  `future_status::timeout` otherwise.
     */
    FUTURES_TEMPLATE(class Range, class Rep, class Period)
    (requires is_range_v<Range> &&is_future_v<range_value_t<Range>>)
        future_status wait_for_all_for(
            std::chrono::duration<Rep, Period> const &timeout_duration,
            Range &&r) {
        using std::begin;
        return wait_for_all_for(begin(r), end(r), timeout_duration);
    }

    /// Wait for a sequence of futures to be ready
    /**
     *  @tparam Fs Range of futures
     *  @tparam Rep Duration Rep
     *  @tparam Period Duration Period
     *  @param timeout_duration Time to wait for
     *  @param fs Future objects
     *
     *  @return `future_status::ready` if all futures got ready.
     *  `future_status::timeout` otherwise.
     */
    FUTURES_TEMPLATE(class... Fs, class Rep, class Period)
    (requires std::conjunction_v<is_future<std::decay_t<Fs>>...>) future_status
        wait_for_all_for(
            std::chrono::duration<Rep, Period> const &timeout_duration,
            Fs &&...fs);


    /// Wait for a sequence of futures to be ready
    /**
     *  @tparam Tuple Tuple of futures
     *  @tparam Rep Duration Rep
     *  @tparam Period Duration Period
     *  @param timeout_duration Time to wait for
     *  @param t Tuple of futures
     *
     *  @return `future_status::ready` if all futures got ready.
     *  `future_status::timeout` otherwise.
     */
    FUTURES_TEMPLATE(class Tuple, class Rep, class Period)
    (requires detail::mp_similar<std::tuple<>, std::decay_t<Tuple>>::value)
        future_status wait_for_all_for(
            std::chrono::duration<Rep, Period> const &timeout_duration,
            Tuple &&t);

    /// Wait for a sequence of futures to be ready
    /**
     *  @tparam Iterator Iterator type in a range of futures
     *  @tparam Clock Time point clock
     *  @tparam Duration Time point duration
     *  @param timeout_time Limit time point
     *  @param first Iterator to the first element in the range
     *  @param last Iterator to one past the last element in the range
     *
     *  @return `future_status::ready` if all futures got ready.
     *  `future_status::timeout` otherwise.
     */
    FUTURES_TEMPLATE(class Iterator, class Clock, class Duration)
    (requires is_future_v<iter_value_t<Iterator>>) future_status
        wait_for_all_until(
            std::chrono::time_point<Clock, Duration> const &timeout_time,
            Iterator first,
            Iterator last);

    /// Wait for a sequence of futures to be ready
    /**
     *  @tparam Range Range of futures
     *  @tparam Clock Time point clock
     *  @tparam Duration Time point duration
     *  @param timeout_time Limit time point
     *  @param r Range of futures
     *
     *  @return `future_status::ready` if all futures got ready.
     *  `future_status::timeout` otherwise.
     */
    FUTURES_TEMPLATE(class Range, class Clock, class Duration)
    (requires is_range_v<Range> &&is_future_v<range_value_t<Range>>)
        future_status wait_for_all_until(
            std::chrono::time_point<Clock, Duration> const &timeout_time,
            Range &&r) {
        using std::begin;
        return wait_for_all_until(begin(r), end(r), timeout_time);
    }

    /// Wait for a sequence of futures to be ready
    /**
     *  @tparam Fs Future objects
     *  @tparam Clock Time point clock
     *  @tparam Duration Time point duration
     *  @param timeout_time Limit time point
     *  @param fs Future objects
     *
     *  @return `future_status::ready` if all futures got ready.
     *  `future_status::timeout` otherwise.
     */
    FUTURES_TEMPLATE(class... Fs, class Clock, class Duration)
    (requires std::conjunction_v<is_future<std::decay_t<Fs>>...>) future_status
        wait_for_all_until(
            std::chrono::time_point<Clock, Duration> const &timeout_time,
            Fs &&...fs);

    /// Wait for a sequence of futures to be ready
    /**
     *  @tparam Tuple Tuple of futures
     *  @tparam Clock Time point clock
     *  @tparam Duration Time point duration
     *  @param timeout_time Limit time point
     *  @param t Tuple of futures
     *
     *  @return `future_status::ready` if all futures got ready.
     *  `future_status::timeout` otherwise.
     */
    FUTURES_TEMPLATE(class Tuple, class Clock, class Duration)
    (requires detail::mp_similar<std::tuple<>, std::decay_t<Tuple>>::value)
        future_status wait_for_all_until(
            std::chrono::time_point<Clock, Duration> const &timeout_time,
            Tuple &&t);

    /** @} */
    /** @} */
} // namespace futures

#include <futures/impl/wait_for_all.hpp>

#endif // FUTURES_WAIT_FOR_ALL_HPP
