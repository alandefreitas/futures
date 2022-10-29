//
// Copyright (c) 2021 alandefreitas (alandefreitas@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//

#ifndef FUTURES_IMPL_WAIT_FOR_ALL_HPP
#define FUTURES_IMPL_WAIT_FOR_ALL_HPP

namespace futures {
    template <
        typename Iterator,
        class Rep,
        class Period FUTURES_REQUIRE_IMPL(
            (is_future_v<iter_value_t<Iterator>>) )>
    std::future_status
    wait_for_all_for(
        std::chrono::duration<Rep, Period> const &timeout_duration,
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

    template <
        typename... Fs,
        class Rep,
        class Period FUTURES_REQUIRE_IMPL(
            (std::conjunction_v<is_future<std::decay_t<Fs>>...>) )>
    std::future_status
    wait_for_all_for(
        std::chrono::duration<Rep, Period> const &timeout_duration,
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

    template <
        class Tuple,
        class Rep,
        class Period FUTURES_REQUIRE_IMPL(
            (detail::mp_similar<std::tuple<>, std::decay_t<Tuple>>::value))>
    std::future_status
    wait_for_all_for(
        std::chrono::duration<Rep, Period> const &timeout_duration,
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

    template <
        typename Iterator,
        class Clock,
        class Duration FUTURES_REQUIRE_IMPL(
            (is_future_v<iter_value_t<Iterator>>) )>
    std::future_status
    wait_for_all_until(
        std::chrono::time_point<Clock, Duration> const &timeout_time,
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

    template <
        typename... Fs,
        class Clock,
        class Duration FUTURES_REQUIRE_IMPL(
            (std::conjunction_v<is_future<std::decay_t<Fs>>...>) )>
    std::future_status
    wait_for_all_until(
        std::chrono::time_point<Clock, Duration> const &timeout_time,
        Fs &&...fs) {
        (fs.wait_until(timeout_time), ...);
        bool all_ready = (is_ready(fs) && ...);
        if (all_ready) {
            return std::future_status::ready;
        } else {
            return std::future_status::timeout;
        }
    }

    template <
        class Tuple,
        class Clock,
        class Duration FUTURES_REQUIRE_IMPL(
            (detail::mp_similar<std::tuple<>, std::decay_t<Tuple>>::value))>
    std::future_status
    wait_for_all_until(
        std::chrono::time_point<Clock, Duration> const &timeout_time,
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
} // namespace futures

#endif // FUTURES_IMPL_WAIT_FOR_ALL_HPP
