//
// Copyright (c) 2021 alandefreitas (alandefreitas@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//

#ifndef FUTURES_IMPL_WAIT_FOR_ANY_HPP
#define FUTURES_IMPL_WAIT_FOR_ANY_HPP

namespace futures {
#ifdef FUTURES_HAS_CONCEPTS
    template <std::input_iterator Iterator>
    requires future_like<iter_value_t<Iterator>>
#else
    template <
        class Iterator,
        std::enable_if_t<is_future_like_v<iter_value_t<Iterator>>, int>>
#endif
    Iterator
    wait_for_any(Iterator first, Iterator last) {
        if (first == last) {
            // is_empty
            return last;
        } else if (std::next(first) == last) {
            // is_single
            first->wait();
            return first;
        } else {
            detail::waiter_for_any waiter(first, last);
            auto ready_future_index = waiter.wait();
            return std::next(first, ready_future_index);
        }
    }

#ifdef FUTURES_HAS_CONCEPTS
    template <future_like... Fs>
#else
    template <
        class... Fs,
        std::enable_if_t<
            detail::conjunction_v<is_future_like<std::decay_t<Fs>>...>,
            int>>
#endif
    std::size_t
    wait_for_any(Fs &&...fs) {
        constexpr std::size_t size = sizeof...(Fs);
        FUTURES_IF_CONSTEXPR (size == 0) {
            // is_empty
            return 0;
        } else FUTURES_IF_CONSTEXPR (size == 1) {
            // is_single
            wait_for_all(std::forward<Fs>(fs)...);
            return 0;
        } else {
            detail::waiter_for_any waiter;
            waiter.add(std::forward<Fs>(fs)...);
            return waiter.wait();
        }
    }

    namespace detail {
        template <class Tuple>
        std::size_t
        wait_for_tuple_any_impl(mp_int<0>, Tuple &&) {
            // empty tuple
            return 0;
        }

        template <class Tuple>
        std::size_t
        wait_for_tuple_any_impl(mp_int<1>, Tuple &&t) {
            // tuple with only one element
            wait_for_all(std::get<0>(std::forward<Tuple>(t)));
            return 0;
        }

        template <class Tuple>
        std::size_t
        wait_for_tuple_any_impl(mp_int<2>, Tuple &&t) {
            // general case
            detail::waiter_for_any waiter;
            detail::tuple_for_each(std::forward<Tuple>(t), [&waiter](auto &f) {
                waiter.add(f);
            });
            return waiter.wait();
        }
    } // namespace detail

#ifdef FUTURES_HAS_CONCEPTS
    template <class Tuple>
    requires detail::mp_similar<std::tuple<>, std::decay_t<Tuple>>::value
#else
    template <
        class Tuple,
        std::enable_if_t<
            detail::mp_similar<std::tuple<>, std::decay_t<Tuple>>::value,
            int>>
#endif
    std::size_t
    wait_for_any(Tuple &&t) {
        constexpr std::size_t size = std::tuple_size<std::decay_t<Tuple>>::value;
        return detail::wait_for_tuple_any_impl(
            boost::mp11::mp_cond<
                boost::mp11::mp_bool<size == 0>,
                boost::mp11::mp_int<0>,
                boost::mp11::mp_bool<size == 1>,
                boost::mp11::mp_int<1>,
                std::true_type,
                boost::mp11::mp_int<2>>{},
            std::forward<Tuple>(t));
    }

#ifdef FUTURES_HAS_CONCEPTS
    template <std::input_iterator Iterator, class Rep, class Period>
    requires future_like<iter_value_t<Iterator>>
#else
    template <
        class Iterator,
        class Rep,
        class Period,
        std::enable_if_t<is_future_like_v<iter_value_t<Iterator>>, int>>
#endif
    Iterator
    wait_for_any_for(
        std::chrono::duration<Rep, Period> const &timeout_duration,
        Iterator first,
        Iterator last) {
        if (first == last) {
            // is_empty
            return last;
        } else if (std::next(first) == last) {
            // is_single
            first->wait_for(timeout_duration);
            return first;
        } else {
            detail::waiter_for_any waiter(first, last);
            auto ready_future_index = waiter.wait_for(timeout_duration);
            return std::next(first, ready_future_index);
        }
    }

#ifdef FUTURES_HAS_CONCEPTS
    template <future_like... Fs, class Rep, class Period>
#else
    template <
        class... Fs,
        class Rep,
        class Period,
        std::enable_if_t<
            detail::conjunction_v<is_future_like<std::decay_t<Fs>>...>,
            int>>
#endif
    std::size_t
    wait_for_any_for(
        std::chrono::duration<Rep, Period> const &timeout_duration,
        Fs &&...fs) {
        constexpr std::size_t size = sizeof...(Fs);
        FUTURES_IF_CONSTEXPR (size == 0) {
            // is_empty
            return 0;
        } else FUTURES_IF_CONSTEXPR (size == 1) {
            // is_single
            wait_for_all_for(timeout_duration, std::forward<Fs>(fs)...);
            return 0;
        } else {
            detail::waiter_for_any waiter;
            waiter.add(std::forward<Fs>(fs)...);
            return waiter.wait_for(timeout_duration);
        }
    }

    namespace detail {
        template <class Tuple, class Rep, class Period>
        std::size_t
        wait_for_tuple_any_for_impl(
            mp_int<0>,
            std::chrono::duration<Rep, Period> const &,
            Tuple &&) {
            // is_empty
            return 0;
        }

        template <class Tuple, class Rep, class Period>
        std::size_t
        wait_for_tuple_any_for_impl(
            mp_int<1>,
            std::chrono::duration<Rep, Period> const &timeout_duration,
            Tuple &&t) {
            // is_single element
            wait_for_all_for(
                timeout_duration,
                std::get<0>(std::forward<Tuple>(t)));
            return !is_ready(std::get<0>(std::forward<Tuple>(t)));
        }

        template <class Tuple, class Rep, class Period>
        std::size_t
        wait_for_tuple_any_for_impl(
            mp_int<2>,
            std::chrono::duration<Rep, Period> const &timeout_duration,
            Tuple &&t) {
            // general case
            detail::waiter_for_any waiter;
            detail::tuple_for_each(std::forward<Tuple>(t), [&waiter](auto &f) {
                waiter.add(f);
            });
            return waiter.wait_for(timeout_duration);
        }
    } // namespace detail

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
            int>>
#endif
    std::size_t
    wait_for_any_for(
        std::chrono::duration<Rep, Period> const &timeout_duration,
        Tuple &&t) {
        constexpr std::size_t size = std::tuple_size<std::decay_t<Tuple>>::value;
        return detail::wait_for_tuple_any_for_impl(
            boost::mp11::mp_cond<
                boost::mp11::mp_bool<size == 0>,
                boost::mp11::mp_int<0>,
                boost::mp11::mp_bool<size == 1>,
                boost::mp11::mp_int<1>,
                std::true_type,
                boost::mp11::mp_int<2>>{},
            timeout_duration,
            std::forward<Tuple>(t));
    }

#ifdef FUTURES_HAS_CONCEPTS
    template <std::input_iterator Iterator, class Clock, class Duration>
    requires future_like<iter_value_t<Iterator>>
#else
    template <
        class Iterator,
        class Clock,
        class Duration,
        std::enable_if_t<is_future_like_v<iter_value_t<Iterator>>, int>>
#endif
    Iterator
    wait_for_any_until(
        std::chrono::time_point<Clock, Duration> const &timeout_time,
        Iterator first,
        Iterator last) {
        if (first == last) {
            // is_empty
            return last;
        } else if (std::next(first) == last) {
            // is_single
            first->wait_until(timeout_time);
            return first;
        } else {
            detail::waiter_for_any waiter(first, last);
            auto ready_future_index = waiter.wait_until(timeout_time);
            return std::next(first, ready_future_index);
        }
    }


#ifdef FUTURES_HAS_CONCEPTS
    template <future_like... Fs, class Clock, class Duration>
#else
    template <
        class... Fs,
        class Clock,
        class Duration,
        std::enable_if_t<
            detail::conjunction_v<is_future_like<std::decay_t<Fs>>...>,
            int>>
#endif
    std::size_t
    wait_for_any_until(
        std::chrono::time_point<Clock, Duration> const &timeout_time,
        Fs &&...fs) {
        constexpr std::size_t size = sizeof...(Fs);
        FUTURES_IF_CONSTEXPR (size == 0) {
            // is_empty
            return 0;
        } else FUTURES_IF_CONSTEXPR (size == 1) {
            // is_single
            wait_for_all_until(timeout_time, std::forward<Fs>(fs)...);
            return 0;
        } else {
            detail::waiter_for_any waiter;
            waiter.add(std::forward<Fs>(fs)...);
            return waiter.wait_until(timeout_time);
        }
    }

    namespace detail {
        template <class Tuple, class Clock, class Duration>
        std::size_t
        wait_for_tuple_any_until_impl(
            mp_int<0>,
            std::chrono::time_point<Clock, Duration> const &,
            Tuple &&) {
            // is_empty
            return 0;
        }

        template <class Tuple, class Clock, class Duration>
        std::size_t
        wait_for_tuple_any_until_impl(
            mp_int<1>,
            std::chrono::time_point<Clock, Duration> const &timeout_time,
            Tuple &&t) {
            // is_single element
            wait_for_all_until(
                timeout_time,
                std::get<0>(std::forward<Tuple>(t)));
            return !is_ready(std::get<0>(std::forward<Tuple>(t)));
        }

        template <class Tuple, class Clock, class Duration>
        std::size_t
        wait_for_tuple_any_until_impl(
            mp_int<2>,
            std::chrono::time_point<Clock, Duration> const &timeout_time,
            Tuple &&t) {
            // general case
            detail::waiter_for_any waiter;
            detail::tuple_for_each(std::forward<Tuple>(t), [&waiter](auto &f) {
                waiter.add(f);
            });
            return waiter.wait_until(timeout_time);
        }
    } // namespace detail

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
            int>>
#endif
    std::size_t
    wait_for_any_until(
        std::chrono::time_point<Clock, Duration> const &timeout_time,
        Tuple &&t) {
        constexpr std::size_t size = std::tuple_size<std::decay_t<Tuple>>::value;
        return detail::wait_for_tuple_any_until_impl(
            boost::mp11::mp_cond<
                boost::mp11::mp_bool<size == 0>,
                boost::mp11::mp_int<0>,
                boost::mp11::mp_bool<size == 1>,
                boost::mp11::mp_int<1>,
                std::true_type,
                boost::mp11::mp_int<2>>{},
            timeout_time,
            std::forward<Tuple>(t));
    }

} // namespace futures

#endif // FUTURES_IMPL_WAIT_FOR_ANY_HPP
