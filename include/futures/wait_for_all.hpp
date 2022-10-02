//
// Copyright (c) 2021 alandefreitas (alandefreitas@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//

#ifndef FUTURES_WAIT_FOR_ALL_HPP
#define FUTURES_WAIT_FOR_ALL_HPP

#include <futures/adaptor/make_ready_future.hpp>
#include <futures/algorithm/traits/is_range.hpp>
#include <futures/algorithm/traits/iter_value.hpp>
#include <futures/algorithm/traits/range_value.hpp>
#include <futures/traits/is_future.hpp>
#include <type_traits>

namespace futures {
    /** @addtogroup futures Futures
     *  @{
     */
    /** @addtogroup waiting Waiting
     *  @{
     */

    /// Wait for a sequence of futures to be ready
    ///
    /// This function waits for all futures in the range [`first`, `last`) to be
    /// ready. It simply waits iteratively for each of the futures to be ready.
    ///
    /// @note This function is adapted from boost::wait_for_all
    ///
    /// @see
    /// [boost.thread
    /// wait_for_all](https://www.boost.org/doc/libs/1_78_0/doc/html/thread/synchronization.html#thread.synchronization.futures.reference.wait_for_all)
    ///
    /// @tparam Iterator Iterator type in a range of futures
    /// @param first Iterator to the first element in the range
    /// @param last Iterator to one past the last element in the range
    template <
        typename Iterator
#ifndef FUTURES_DOXYGEN
        ,
        typename std::enable_if_t<
            // clang-format off
            is_future_v<iter_value_t<Iterator>>
            // clang-format on
            ,
            int> = 0
#endif
        >
    void
    wait_for_all(Iterator first, Iterator last) {
        for (Iterator it = first; it != last; ++it) {
            it->wait();
        }
    }

    /// Wait for a sequence of futures to be ready
    ///
    /// This function waits for all futures in the range `r` to be ready.
    /// It simply waits iteratively for each of the futures to be ready.
    ///
    /// @note This function is adapted from boost::wait_for_all
    ///
    /// @see
    /// [boost.thread
    /// wait_for_all](https://www.boost.org/doc/libs/1_78_0/doc/html/thread/synchronization.html#thread.synchronization.futures.reference.wait_for_all)
    ///
    /// @tparam Range A range of futures type
    /// @param r Range of futures
    template <
        typename Range
#ifndef FUTURES_DOXYGEN
        ,
        typename std::enable_if_t<
            // clang-format off
            is_range_v<Range> &&
            is_future_v<range_value_t<Range>>
            // clang-format on
            ,
            int> = 0
#endif
        >
    void
    wait_for_all(Range &&r) {
        using std::begin;
        wait_for_all(begin(r), end(r));
    }

    /// Wait for a sequence of futures to be ready
    ///
    /// This function waits for all specified futures `fs`... to be ready.
    ///
    /// It creates a compile-time fixed-size data structure to store references
    /// to all of the futures and then waits for each of the futures to be
    /// ready.
    ///
    /// @note This function is adapted from boost::wait_for_all
    ///
    /// @see
    /// [boost.thread
    /// wait_for_all](https://www.boost.org/doc/libs/1_78_0/doc/html/thread/synchronization.html#thread.synchronization.futures.reference.wait_for_all)
    ///
    /// @tparam Fs A list of future types
    /// @param fs A list of future objects
    template <
        typename... Fs
#ifndef FUTURES_DOXYGEN
        ,
        typename std::enable_if_t<
            // clang-format off
            std::conjunction_v<is_future<std::decay_t<Fs>>...>
            // clang-format on
            ,
            int> = 0
#endif
        >
    void
    wait_for_all(Fs &&...fs) {
        (fs.wait(), ...);
    }

    /// Wait for a sequence of futures to be ready
    template <
        class Tuple
#ifndef FUTURES_DOXYGEN
        ,
        typename std::enable_if_t<
            // clang-format off
            detail::is_tuple_v<std::decay_t<Tuple>>
            // clang-format on
            ,
            int> = 0
#endif
        >
    void
    wait_for_all(Tuple &&t) {
        tuple_for_each(std::forward<Tuple>(t), [](auto &f) { f.wait(); });
    }

    /// Wait for a sequence of futures to be ready
    ///
    /// @tparam Iterator Iterator type in a range of futures
    /// @tparam Rep Duration Rep
    /// @tparam Period Duration Period
    /// @param timeout_duration Time to wait for
    /// @param first Iterator to the first element in the range
    /// @param last Iterator to one past the last element in the range
    ///
    /// @return `std::future_status::ready` if all futures got ready.
    /// `std::future_status::timeout` otherwise.
    template <
        typename Iterator,
        class Rep,
        class Period
#ifndef FUTURES_DOXYGEN
        ,
        typename std::enable_if_t<
            // clang-format off
            is_future_v<iter_value_t<Iterator>>
            // clang-format on
            ,
            int> = 0
#endif
        >
    std::future_status
    wait_for_all_for(
        const std::chrono::duration<Rep, Period> &timeout_duration,
        Iterator first,
        Iterator last) {
        auto until_tp = std::chrono::system_clock::now() + timeout_duration;
        for (Iterator it = first; it != last; ++it) {
            it->wait_until(until_tp);
        }
        if (std::all_of(first, last, [](auto &f) { return is_ready(f); })) {
            return std::future_status::ready;
        } else {
            return std::future_status::timeout;
        }
    }

    /// Wait for a sequence of futures to be ready
    ///
    /// @tparam Range Range of futures
    /// @tparam Rep Duration Rep
    /// @tparam Period Duration Period
    /// @param timeout_duration Time to wait for
    /// @param r Range of futures
    ///
    /// @return `std::future_status::ready` if all futures got ready.
    /// `std::future_status::timeout` otherwise.
    template <
        class Range,
        class Rep,
        class Period
#ifndef FUTURES_DOXYGEN
        ,
        typename std::enable_if_t<
            // clang-format off
            is_range_v<Range> &&
            is_future_v<range_value_t<Range>>
            // clang-format on
            ,
            int> = 0
#endif
        >
    std::future_status
    wait_for_all_for(
        const std::chrono::duration<Rep, Period> &timeout_duration,
        Range &&r) {
        using std::begin;
        return wait_for_all_for(begin(r), end(r), timeout_duration);
    }

    /// Wait for a sequence of futures to be ready
    ///
    /// @tparam Fs Range of futures
    /// @tparam Rep Duration Rep
    /// @tparam Period Duration Period
    /// @param timeout_duration Time to wait for
    /// @param fs Future objects
    ///
    /// @return `std::future_status::ready` if all futures got ready.
    /// `std::future_status::timeout` otherwise.
    template <
        typename... Fs,
        class Rep,
        class Period
#ifndef FUTURES_DOXYGEN
        ,
        typename std::enable_if_t<
            // clang-format off
            std::conjunction_v<is_future<std::decay_t<Fs>>...>
            // clang-format on
            ,
            int> = 0
#endif
        >
    std::future_status
    wait_for_all_for(
        const std::chrono::duration<Rep, Period> &timeout_duration,
        Fs &&...fs) {
        auto until_tp = std::chrono::system_clock::now() + timeout_duration;
        (fs.wait_until(until_tp), ...);
        bool all_ready = (is_ready(fs) && ...);
        if (all_ready) {
            return std::future_status::ready;
        } else {
            return std::future_status::timeout;
        }
    }


    /// Wait for a sequence of futures to be ready
    ///
    /// @tparam Tuple Tuple of futures
    /// @tparam Rep Duration Rep
    /// @tparam Period Duration Period
    /// @param timeout_duration Time to wait for
    /// @param t Tuple of futures
    ///
    /// @return `std::future_status::ready` if all futures got ready.
    /// `std::future_status::timeout` otherwise.
    template <
        class Tuple,
        class Rep,
        class Period
#ifndef FUTURES_DOXYGEN
        ,
        typename std::enable_if_t<
            // clang-format off
            detail::is_tuple_v<std::decay_t<Tuple>>
            // clang-format on
            ,
            int> = 0
#endif
        >
    std::future_status
    wait_for_all_for(
        const std::chrono::duration<Rep, Period> &timeout_duration,
        Tuple &&t) {
        auto until_tp = std::chrono::system_clock::now() + timeout_duration;
        tuple_for_each(std::forward<Tuple>(t), [&until_tp](auto &f) {
            f.wait_until(until_tp);
        });
        bool all_ready = tuple_all_of(std::forward<Tuple>(t), [](auto &f) {
            is_ready(f);
        });
        if (all_ready) {
            return std::future_status::ready;
        } else {
            return std::future_status::timeout;
        }
    }


    /// Wait for a sequence of futures to be ready
    ///
    /// @tparam Iterator Iterator type in a range of futures
    /// @tparam Clock Time point clock
    /// @tparam Duration Time point duration
    /// @param timeout_time Limit time point
    /// @param first Iterator to the first element in the range
    /// @param last Iterator to one past the last element in the range
    ///
    /// @return `std::future_status::ready` if all futures got ready.
    /// `std::future_status::timeout` otherwise.
    template <
        typename Iterator,
        class Clock,
        class Duration
#ifndef FUTURES_DOXYGEN
        ,
        typename std::enable_if_t<
            // clang-format off
            is_future_v<iter_value_t<Iterator>>
            // clang-format on
            ,
            int> = 0
#endif
        >
    std::future_status
    wait_for_all_until(
        const std::chrono::time_point<Clock, Duration> &timeout_time,
        Iterator first,
        Iterator last) {
        for (Iterator it = first; it != last; ++it) {
            it->wait_until(timeout_time);
        }
        if (std::all_of(first, last, [](auto &f) { return is_ready(f); })) {
            return std::future_status::ready;
        } else {
            return std::future_status::timeout;
        }
    }

    /// Wait for a sequence of futures to be ready
    ///
    /// @tparam Range Range of futures
    /// @tparam Clock Time point clock
    /// @tparam Duration Time point duration
    /// @param timeout_time Limit time point
    /// @param r Range of futures
    ///
    /// @return `std::future_status::ready` if all futures got ready.
    /// `std::future_status::timeout` otherwise.
    template <
        class Range,
        class Clock,
        class Duration
#ifndef FUTURES_DOXYGEN
        ,
        typename std::enable_if_t<
            // clang-format off
            is_range_v<Range> &&
            is_future_v<range_value_t<Range>>
            // clang-format on
            ,
            int> = 0
#endif
        >
    std::future_status
    wait_for_all_until(
        const std::chrono::time_point<Clock, Duration> &timeout_time,
        Range &&r) {
        using std::begin;
        return wait_for_all_until(begin(r), end(r), timeout_time);
    }

    /// Wait for a sequence of futures to be ready
    ///
    /// @tparam Fs Future objects
    /// @tparam Clock Time point clock
    /// @tparam Duration Time point duration
    /// @param timeout_time Limit time point
    /// @param fs Future objects
    ///
    /// @return `std::future_status::ready` if all futures got ready.
    /// `std::future_status::timeout` otherwise.
    template <
        typename... Fs,
        class Clock,
        class Duration
#ifndef FUTURES_DOXYGEN
        ,
        typename std::enable_if_t<
            // clang-format off
            std::conjunction_v<is_future<std::decay_t<Fs>>...>
            // clang-format on
            ,
            int> = 0
#endif
        >
    std::future_status
    wait_for_all_until(
        const std::chrono::time_point<Clock, Duration> &timeout_time,
        Fs &&...fs) {
        (fs.wait_until(timeout_time), ...);
        bool all_ready = (is_ready(fs) && ...);
        if (all_ready) {
            return std::future_status::ready;
        } else {
            return std::future_status::timeout;
        }
    }


    /// Wait for a sequence of futures to be ready
    ///
    /// @tparam Tuple Tuple of futures
    /// @tparam Clock Time point clock
    /// @tparam Duration Time point duration
    /// @param timeout_time Limit time point
    /// @param t Tuple of futures
    ///
    /// @return `std::future_status::ready` if all futures got ready.
    /// `std::future_status::timeout` otherwise.
    template <
        class Tuple,
        class Clock,
        class Duration
#ifndef FUTURES_DOXYGEN
        ,
        typename std::enable_if_t<
            // clang-format off
            detail::is_tuple_v<std::decay_t<Tuple>>
            // clang-format on
            ,
            int> = 0
#endif
        >
    std::future_status
    wait_for_all_until(
        const std::chrono::time_point<Clock, Duration> &timeout_time,
        Tuple &&t) {
        tuple_for_each(std::forward<Tuple>(t), [&timeout_time](auto &f) {
            f.wait_until(timeout_time);
        });
        bool all_ready = tuple_all_of(std::forward<Tuple>(t), [](auto &f) {
            is_ready(f);
        });
        if (all_ready) {
            return std::future_status::ready;
        } else {
            return std::future_status::timeout;
        }
    }

    /** @} */
    /** @} */
} // namespace futures

#endif // FUTURES_WAIT_FOR_ALL_HPP
