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
#include <futures/traits/is_future_like.hpp>
#include <futures/detail/traits/std_type_traits.hpp>
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
#ifdef FUTURES_HAS_CONCEPTS
    template <std::input_iterator Iterator>
    requires future_like<std::iter_value_t<Iterator>>
#else
    template <
        class Iterator,
        std::enable_if_t<is_future_like_v<iter_value_t<Iterator>>, int> = 0>
#endif
    void
    wait_for_all(Iterator first, Iterator last) {
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
#ifdef FUTURES_HAS_CONCEPTS
    template <std::ranges::range Range>
    requires future_like<std::ranges::range_value_t<Range>>
#else
    template <
        class Range,
        std::enable_if_t<
            is_range_v<Range> && is_future_like_v<range_value_t<Range>>,
            int>
        = 0>
#endif
    void
    wait_for_all(Range &&r) {
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
#ifdef FUTURES_HAS_CONCEPTS
    template <future_like... Fs>
#else
    template <
        class... Fs,
        std::enable_if_t<
            detail::conjunction_v<is_future_like<std::decay_t<Fs>>...>,
            int>
        = 0>
#endif
    void
    wait_for_all(Fs &&...fs) {
#ifdef BOOST_NO_CXX17_FOLD_EXPRESSIONS
        detail::tuple_for_each(
            std::forward_as_tuple(std::forward<Fs>(fs)...),
            [](auto &&f) { f.wait(); });
#else
        (fs.wait(), ...);
#endif
    }

    /// Wait for a sequence of futures to be ready
#ifdef FUTURES_HAS_CONCEPTS
    template <class Tuple>
    requires detail::mp_similar<std::tuple<>, std::decay_t<Tuple>>::value
#else
    template <
        class Tuple,
        std::enable_if_t<
            detail::mp_similar<std::tuple<>, std::decay_t<Tuple>>::value,
            int>
        = 0>
#endif
    void
    wait_for_all(Tuple &&t) {
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
#ifdef FUTURES_HAS_CONCEPTS
    template <class Iterator, class Rep, class Period>
    requires is_future_like_v<iter_value_t<Iterator>>
#else
    template <
        class Iterator,
        class Rep,
        class Period,
        std::enable_if_t<is_future_like_v<iter_value_t<Iterator>>, int> = 0>
#endif
    future_status
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
#ifdef FUTURES_HAS_CONCEPTS
    template <class Range, class Rep, class Period>
    requires is_range_v<Range> && is_future_like_v<range_value_t<Range>>
#else
    template <
        class Range,
        class Rep,
        class Period,
        std::enable_if_t<
            is_range_v<Range> && is_future_like_v<range_value_t<Range>>,
            int>
        = 0>
#endif
    future_status
    wait_for_all_for(
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
#ifdef FUTURES_HAS_CONCEPTS
    template <future_like... Fs, class Rep, class Period>
#else
    template <
        class... Fs,
        class Rep,
        class Period,
        std::enable_if_t<
            detail::conjunction_v<is_future_like<std::decay_t<Fs>>...>,
            int>
        = 0>
#endif
    future_status
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
#ifdef FUTURES_HAS_CONCEPTS
    template <class Tuple, class Rep, class Period>
    requires detail::mp_similar<std::tuple<>, std::decay_t<Tuple>>::value
#else
    template <
        class Tuple,
        class Rep,
        class Period,
        std::enable_if_t<
            detail::mp_similar<std::tuple<>, std::decay_t<Tuple>>::value,
            int>
        = 0>
#endif
    future_status
    wait_for_all_for(
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
#ifdef FUTURES_HAS_CONCEPTS
    template <std::input_iterator Iterator, class Clock, class Duration>
    requires future_like<std::iter_value_t<Iterator>>
#else
    template <
        class Iterator,
        class Clock,
        class Duration,
        std::enable_if_t<is_future_like_v<iter_value_t<Iterator>>, int> = 0>
#endif
    future_status
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
#ifdef FUTURES_HAS_CONCEPTS
    template <std::ranges::range Range, class Clock, class Duration>
    requires future_like<range_value_t<Range>>
#else
    template <
        class Range,
        class Clock,
        class Duration,
        std::enable_if_t<
            is_range_v<Range> && is_future_like_v<range_value_t<Range>>,
            int>
        = 0>
#endif
    future_status
    wait_for_all_until(
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
#ifdef FUTURES_HAS_CONCEPTS
    template <future_like... Fs, class Clock, class Duration>
#else
    template <
        class... Fs,
        class Clock,
        class Duration,
        std::enable_if_t<
            detail::conjunction_v<is_future_like<std::decay_t<Fs>>...>,
            int>
        = 0>
#endif
    future_status
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
#ifdef FUTURES_HAS_CONCEPTS
    template <class Tuple, class Clock, class Duration>
    requires detail::mp_similar<std::tuple<>, std::decay_t<Tuple>>::value
#else
    template <
        class Tuple,
        class Clock,
        class Duration,
        std::enable_if_t<
            detail::mp_similar<std::tuple<>, std::decay_t<Tuple>>::value,
            int>
        = 0>
#endif
    future_status
    wait_for_all_until(
        std::chrono::time_point<Clock, Duration> const &timeout_time,
        Tuple &&t);

    /** @} */
    /** @} */
} // namespace futures

#include <futures/impl/wait_for_all.hpp>

#endif // FUTURES_WAIT_FOR_ALL_HPP
