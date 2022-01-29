//
// Copyright (c) 2021 alandefreitas (alandefreitas@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//

#ifndef FUTURES_WAIT_FOR_ANY_H
#define FUTURES_WAIT_FOR_ANY_H

#include <futures/algorithm/traits/iter_value.hpp>
#include <futures/algorithm/traits/is_range.hpp>
#include <futures/algorithm/traits/range_value.hpp>
#include <futures/futures/traits/is_future.hpp>
#include <futures/futures/detail/waiter_for_any.hpp>
#include <type_traits>

namespace futures {
    /** \addtogroup futures Futures
     *  @{
     */
    /** \addtogroup waiting Waiting
     *  @{
     */

    /// \brief Wait for any future in a sequence to be ready
    ///
    /// This function waits for any future in the range [`first`, `last`) to be
    /// ready.
    ///
    /// Unlike @ref wait_for_all, this function requires special data structures
    /// to allow that to happen without blocking.
    ///
    /// \note This function is adapted from `boost::wait_for_any`
    ///
    /// \see
    /// [boost.thread
    /// wait_for_any](https://www.boost.org/doc/libs/1_78_0/doc/html/thread/synchronization.html#thread.synchronization.futures.reference.wait_for_any)
    ///
    /// \tparam Iterator Iterator type in a range of futures
    /// \param first Iterator to the first element in the range
    /// \param last Iterator to one past the last element in the range
    /// \return Iterator to the first future that got ready
    template <
        typename Iterator
#ifndef FUTURES_DOXYGEN
        ,
        typename std::
            enable_if_t<is_future_v<iter_value_t<Iterator>>, int> = 0
#endif
        >
    Iterator
    wait_for_any(Iterator first, Iterator last) {
        if (const bool is_empty = first == last; is_empty) {
            return last;
        } else if (const bool is_single = std::next(first) == last; is_single) {
            first->wait();
            return first;
        } else {
            detail::waiter_for_any waiter(first, last);
            auto ready_future_index = waiter.wait();
            return std::next(first, ready_future_index);
        }
    }

    /// \brief Wait for any future in a sequence to be ready
    ///
    /// This function waits for any future in the range `r` to be ready.
    /// This function requires special data structures to allow that to happen
    /// without blocking.
    ///
    /// \note This function is adapted from `boost::wait_for_any`
    ///
    /// \see
    /// [boost.thread
    /// wait_for_any](https://www.boost.org/doc/libs/1_78_0/doc/html/thread/synchronization.html#thread.synchronization.futures.reference.wait_for_any)
    ///
    /// \tparam Iterator A range of futures type
    /// \param r Range of futures
    /// \return Iterator to the first future that got ready
    template <
        typename Range
#ifndef FUTURES_DOXYGEN
        ,
        typename std::enable_if_t<
            is_range_v<Range> && is_future_v<range_value_t<Range>>,
            int> = 0
#endif
        >
    iterator_t<Range>
    wait_for_any(Range &&r) {
        return wait_for_any(std::begin(r), std::end(r));
    }

    /// \brief Wait for any future in a sequence to be ready
    ///
    /// This function waits for all specified futures `fs`... to be ready.
    ///
    /// \note This function is adapted from `boost::wait_for_any`
    ///
    /// \see
    /// [boost.thread
    /// wait_for_any](https://www.boost.org/doc/libs/1_78_0/doc/html/thread/synchronization.html#thread.synchronization.futures.reference.wait_for_any)
    ///
    /// \tparam Fs A list of future types
    /// \param fs A list of future objects
    /// \return Index of the first future that got ready
    template <
        typename... Fs
#ifndef FUTURES_DOXYGEN
        ,
        typename std::enable_if_t<
            std::conjunction_v<is_future<std::decay_t<Fs>>...>,
            int> = 0
#endif
        >
    std::size_t
    wait_for_any(Fs &&...fs) {
        constexpr std::size_t size = sizeof...(Fs);
        if constexpr (const bool is_empty = size == 0; is_empty) {
            return 0;
        } else if constexpr (const bool is_single = size == 1; is_single) {
            wait_for_all(std::forward<Fs>(fs)...);
            return 0;
        } else {
            detail::waiter_for_any waiter;
            waiter.add(std::forward<Fs>(fs)...);
            return waiter.wait();
        }
    }

    /** @} */
    /** @} */
} // namespace futures

#endif // FUTURES_WAIT_FOR_ANY_H
