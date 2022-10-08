//
// Copyright (c) 2021 alandefreitas (alandefreitas@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//

#ifndef FUTURES_WAIT_FOR_ANY_HPP
#define FUTURES_WAIT_FOR_ANY_HPP

#include <futures/algorithm/traits/is_range.hpp>
#include <futures/algorithm/traits/iter_value.hpp>
#include <futures/algorithm/traits/range_value.hpp>
#include <futures/traits/is_future.hpp>
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
    ///
    /// This function waits for any future in the range [`first`, `last`) to be
    /// ready.
    ///
    /// Unlike @ref wait_for_all, this function requires special data structures
    /// to allow that to happen without blocking.
    ///
    /// For disjunctions, we have few options:
    /// - If the input futures support external notifiers:
    ///     - Attach continuations to notify when a task is over
    /// - If the input futures do not have lazy continuations:
    ///     - Polling in a busy loop until one of the futures is ready
    ///     - Polling with exponential backoffs until one of the futures is
    ///     ready
    ///     - Launching n continuation tasks that set a promise when one of the
    ///   futures is ready
    ///     - Hybrids, usually polling for short tasks and launching threads for
    ///   other tasks
    /// - If the input futures are mixed in regards to lazy continuations:
    ///     - Mix the strategies above, depending on each input future
    ///
    /// If the thresholds for these strategies are reasonable, this should be
    /// efficient for futures with or without lazy continuations.
    ///
    /// @note This function is adapted from `boost::wait_for_any`
    ///
    /// @see
    /// [boost.thread
    /// wait_for_any](https://www.boost.org/doc/libs/1_78_0/doc/html/thread/synchronization.html#thread.synchronization.futures.reference.wait_for_any)
    ///
    /// @tparam Iterator Iterator type in a range of futures
    /// @param first Iterator to the first element in the range
    /// @param last Iterator to one past the last element in the range
    /// @return Iterator to the first future that got ready
    template <
        typename Iterator
#ifndef FUTURES_DOXYGEN
        ,
        typename std::enable_if_t<is_future_v<iter_value_t<Iterator>>, int> = 0
#endif
        >
    Iterator
    wait_for_any(Iterator first, Iterator last) {
        if (bool const is_empty = first == last; is_empty) {
            return last;
        } else if (bool const is_single = std::next(first) == last; is_single) {
            first->wait();
            return first;
        } else {
            detail::waiter_for_any waiter(first, last);
            auto ready_future_index = waiter.wait();
            return std::next(first, ready_future_index);
        }
    }


    /// Wait for any future in a sequence to be ready
    ///
    /// This function waits for any future in the range `r` to be ready.
    /// This function requires special data structures to allow that to happen
    /// without blocking.
    ///
    /// @note This function is adapted from `boost::wait_for_any`
    ///
    /// @see
    /// [boost.thread
    /// wait_for_any](https://www.boost.org/doc/libs/1_78_0/doc/html/thread/synchronization.html#thread.synchronization.futures.reference.wait_for_any)
    ///
    /// @tparam Iterator A range of futures type
    /// @param r Range of futures
    /// @return Iterator to the first future that got ready
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
            int>
        = 0
#endif
        >
    iterator_t<Range>
    wait_for_any(Range &&r) {
        return wait_for_any(std::begin(r), std::end(r));
    }

    /// Wait for any future in a sequence to be ready
    ///
    /// This function waits for all specified futures `fs`... to be ready.
    ///
    /// @note This function is adapted from `boost::wait_for_any`
    ///
    /// @see
    /// [boost.thread
    /// wait_for_any](https://www.boost.org/doc/libs/1_78_0/doc/html/thread/synchronization.html#thread.synchronization.futures.reference.wait_for_any)
    ///
    /// @tparam Fs A list of future types
    /// @param fs A list of future objects
    /// @return Index of the first future that got ready
    template <
        typename... Fs
#ifndef FUTURES_DOXYGEN
        ,
        typename std::
            enable_if_t<std::conjunction_v<is_future<std::decay_t<Fs>>...>, int>
        = 0
#endif
        >
    std::size_t
    wait_for_any(Fs &&...fs) {
        constexpr std::size_t size = sizeof...(Fs);
        if constexpr (bool const is_empty = size == 0; is_empty) {
            return 0;
        } else if constexpr (bool const is_single = size == 1; is_single) {
            wait_for_all(std::forward<Fs>(fs)...);
            return 0;
        } else {
            detail::waiter_for_any waiter;
            waiter.add(std::forward<Fs>(fs)...);
            return waiter.wait();
        }
    }

    /// Wait for any future in a tuple to be ready
    ///
    /// This function waits for all specified futures `fs`... to be ready.
    ///
    /// @note This function is adapted from `boost::wait_for_any`
    ///
    /// @see
    /// [boost.thread
    /// wait_for_any](https://www.boost.org/doc/libs/1_78_0/doc/html/thread/synchronization.html#thread.synchronization.futures.reference.wait_for_any)
    ///
    /// @tparam Fs A list of future types
    ///
    /// @param t A list of future objects
    ///
    /// @return Index of the first future that got ready
    template <
        class Tuple
#ifndef FUTURES_DOXYGEN
        ,
        typename std::enable_if_t<
            // clang-format off
            detail::is_tuple_v<std::decay_t<Tuple>>
            // clang-format on
            ,
            int>
        = 0
#endif
        >
    std::size_t
    wait_for_any(Tuple &&t) {
        constexpr std::size_t size = std::tuple_size_v<std::decay_t<Tuple>>;
        if constexpr (bool const is_empty = size == 0; is_empty) {
            return 0;
        } else if constexpr (bool const is_single = size == 1; is_single) {
            wait_for_all(std::get<0>(std::forward<Tuple>(t)));
            return 0;
        } else {
            detail::waiter_for_any waiter;
            detail::tuple_for_each(std::forward<Tuple>(t), [&waiter](auto &f) {
                waiter.add(f);
            });
            return waiter.wait();
        }
    }

    /// Wait for any future in a sequence to be ready
    ///
    /// @tparam Iterator Iterator type in a range of futures
    /// @tparam Rep Duration Rep
    /// @tparam Period Duration Period
    /// @param timeout_duration Time to wait for
    /// @param first Iterator to the first element in the range
    /// @param last Iterator to one past the last element in the range
    ///
    /// @return Iterator to the future which got ready
    template <
        typename Iterator,
        class Rep,
        class Period
#ifndef FUTURES_DOXYGEN
        ,
        typename std::enable_if_t<is_future_v<iter_value_t<Iterator>>, int> = 0
#endif
        >
    Iterator
    wait_for_any_for(
        std::chrono::duration<Rep, Period> const &timeout_duration,
        Iterator first,
        Iterator last) {
        if (bool const is_empty = first == last; is_empty) {
            return last;
        } else if (bool const is_single = std::next(first) == last; is_single) {
            first->wait_for(timeout_duration);
            return first;
        } else {
            detail::waiter_for_any waiter(first, last);
            auto ready_future_index = waiter.wait_for(timeout_duration);
            return std::next(first, ready_future_index);
        }
    }

    /// Wait for any future in a sequence to be ready
    ///
    /// @tparam Range Iterator type in a range of futures
    /// @tparam Rep Duration Rep
    /// @tparam Period Duration Period
    /// @param timeout_duration Time to wait for
    /// @param r Range of futures
    ///
    /// @return Iterator to the future which got ready
    template <
        typename Range,
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
            int>
        = 0
#endif
        >
    iterator_t<Range>
    wait_for_any_for(
        std::chrono::duration<Rep, Period> const &timeout_duration,
        Range &&r) {
        return wait_for_any_for(timeout_duration, std::begin(r), std::end(r));
    }


    /// Wait for any future in a sequence to be ready
    ///
    /// @tparam Fs Future types
    /// @tparam Rep Duration Rep
    /// @tparam Period Duration Period
    /// @param timeout_duration Time to wait for
    /// @param fs Future objects
    ///
    /// @return Index of the future which got ready
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
            int>
        = 0
#endif
        >
    std::size_t
    wait_for_any_for(
        std::chrono::duration<Rep, Period> const &timeout_duration,
        Fs &&...fs) {
        constexpr std::size_t size = sizeof...(Fs);
        if constexpr (bool const is_empty = size == 0; is_empty) {
            return 0;
        } else if constexpr (bool const is_single = size == 1; is_single) {
            wait_for_all_for(timeout_duration, std::forward<Fs>(fs)...);
            return 0;
        } else {
            detail::waiter_for_any waiter;
            waiter.add(std::forward<Fs>(fs)...);
            return waiter.wait_for(timeout_duration);
        }
    }

    /// Wait for any future in a sequence to be ready
    ///
    /// @tparam Tuple Tuple of futures
    /// @tparam Rep Duration Rep
    /// @tparam Period Duration Period
    /// @param timeout_duration Time to wait for
    /// @param t tuple of futures
    ///
    /// @return Index of the future which got ready
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
            int>
        = 0
#endif
        >
    std::size_t
    wait_for_any_for(
        std::chrono::duration<Rep, Period> const &timeout_duration,
        Tuple &&t) {
        constexpr std::size_t size = std::tuple_size_v<std::decay_t<Tuple>>;
        if constexpr (bool const is_empty = size == 0; is_empty) {
            return 0;
        } else if constexpr (bool const is_single = size == 1; is_single) {
            wait_for_all_for(
                timeout_duration,
                std::get<0>(std::forward<Tuple>(t)));
            return 0;
        } else {
            detail::waiter_for_any waiter;
            detail::tuple_for_each(std::forward<Tuple>(t), [&waiter](auto &f) {
                waiter.add(f);
            });
            return waiter.wait_for(timeout_duration);
        }
    }

    /// Wait for any future in a sequence to be ready
    ///
    /// @tparam Iterator Iterator type in a range of futures
    /// @tparam Clock Time point clock
    /// @tparam Duration Time point duration
    /// @param timeout_time Limit time point
    /// @param first Iterator to the first element in the range
    /// @param last Iterator to one past the last element in the range
    ///
    /// @return Iterator to the future which got ready
    template <
        typename Iterator,
        class Clock,
        class Duration
#ifndef FUTURES_DOXYGEN
        ,
        typename std::enable_if_t<is_future_v<iter_value_t<Iterator>>, int> = 0
#endif
        >
    Iterator
    wait_for_any_until(
        std::chrono::time_point<Clock, Duration> const &timeout_time,
        Iterator first,
        Iterator last) {
        if (bool const is_empty = first == last; is_empty) {
            return last;
        } else if (bool const is_single = std::next(first) == last; is_single) {
            first->wait_until(timeout_time);
            return first;
        } else {
            detail::waiter_for_any waiter(first, last);
            auto ready_future_index = waiter.wait_until(timeout_time);
            return std::next(first, ready_future_index);
        }
    }


    /// Wait for any future in a sequence to be ready
    ///
    /// @tparam Range Range of futures
    /// @tparam Clock Time point clock
    /// @tparam Duration Time point duration
    /// @param timeout_time Limit time point
    /// @param r Range of futures
    ///
    /// @return Iterator to the future which got ready
    template <
        typename Range,
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
            int>
        = 0
#endif
        >
    iterator_t<Range>
    wait_for_any_until(
        std::chrono::time_point<Clock, Duration> const &timeout_time,
        Range &&r) {
        return wait_for_any_until(timeout_time, std::begin(r), std::end(r));
    }


    /// Wait for any future in a sequence to be ready
    ///
    /// @tparam Fs Future types
    /// @tparam Clock Time point clock
    /// @tparam Duration Time point duration
    /// @param timeout_time Limit time point
    /// @param fs Future objects
    ///
    /// @return Index of the future which got ready
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
            int>
        = 0
#endif
        >
    std::size_t
    wait_for_any_until(
        std::chrono::time_point<Clock, Duration> const &timeout_time,
        Fs &&...fs) {
        constexpr std::size_t size = sizeof...(Fs);
        if constexpr (bool const is_empty = size == 0; is_empty) {
            return 0;
        } else if constexpr (bool const is_single = size == 1; is_single) {
            wait_for_all_until(timeout_time, std::forward<Fs>(fs)...);
            return 0;
        } else {
            detail::waiter_for_any waiter;
            waiter.add(std::forward<Fs>(fs)...);
            return waiter.wait_until(timeout_time);
        }
    }

    /// Wait for any future in a sequence to be ready
    ///
    /// @tparam Tuple Tuple of future types
    /// @tparam Clock Time point clock
    /// @tparam Duration Time point duration
    /// @param timeout_time Limit time point
    /// @param t Tuple of future objects
    ///
    /// @return Index of the future which got ready
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
            int>
        = 0
#endif
        >
    std::size_t
    wait_for_any_until(
        std::chrono::time_point<Clock, Duration> const &timeout_time,
        Tuple &&t) {
        constexpr std::size_t size = std::tuple_size_v<std::decay_t<Tuple>>;
        if constexpr (bool const is_empty = size == 0; is_empty) {
            return 0;
        } else if constexpr (bool const is_single = size == 1; is_single) {
            wait_for_all_until(
                timeout_time,
                std::get<0>(std::forward<Tuple>(t)));
            return 0;
        } else {
            detail::waiter_for_any waiter;
            detail::tuple_for_each(std::forward<Tuple>(t), [&waiter](auto &f) {
                waiter.add(f);
            });
            return waiter.wait_until(timeout_time);
        }
    }


    /** @} */
    /** @} */
} // namespace futures

#endif // FUTURES_WAIT_FOR_ANY_HPP
