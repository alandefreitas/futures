//
// Copyright (c) 2021 alandefreitas (alandefreitas@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//

#ifndef FUTURES_IMPL_WAIT_FOR_ANY_HPP
#define FUTURES_IMPL_WAIT_FOR_ANY_HPP

namespace futures {
    template <
        typename Iterator,
        typename std::enable_if_t<is_future_v<iter_value_t<Iterator>>, int>>
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

    template <
        typename... Fs,
        typename std::
            enable_if_t<std::conjunction_v<is_future<std::decay_t<Fs>>...>, int>>
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

    template <
        class Tuple,
        typename std::enable_if_t<
            // clang-format off
            detail::mp_similar<std::tuple<>, std::decay_t<Tuple>>::value
            // clang-format on
            ,
            int>>
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

    template <
        typename Iterator,
        class Rep,
        class Period,
        typename std::enable_if_t<is_future_v<iter_value_t<Iterator>>, int>>
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

    template <
        typename... Fs,
        class Rep,
        class Period,
        typename std::enable_if_t<
            // clang-format off
            std::conjunction_v<is_future<std::decay_t<Fs>>...>
            // clang-format on
            ,
            int>>
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

    template <
        class Tuple,
        class Rep,
        class Period,
        typename std::enable_if_t<
            // clang-format off
            detail::mp_similar<std::tuple<>, std::decay_t<Tuple>>::value
            // clang-format on
            ,
            int>>
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

    template <
        typename Iterator,
        class Clock,
        class Duration,
        typename std::enable_if_t<is_future_v<iter_value_t<Iterator>>, int>>
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

    template <
        typename... Fs,
        class Clock,
        class Duration,
        typename std::enable_if_t<
            // clang-format off
            std::conjunction_v<is_future<std::decay_t<Fs>>...>
            // clang-format on
            ,
            int>>
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

    template <
        class Tuple,
        class Clock,
        class Duration,
        typename std::enable_if_t<
            // clang-format off
            detail::mp_similar<std::tuple<>, std::decay_t<Tuple>>::value
            // clang-format on
            ,
            int>>
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

} // namespace futures

#endif // FUTURES_IMPL_WAIT_FOR_ANY_HPP
