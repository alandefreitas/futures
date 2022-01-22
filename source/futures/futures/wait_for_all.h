//
// Copyright (c) 2021 alandefreitas (alandefreitas@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//

#ifndef FUTURES_WAIT_FOR_ALL_H
#define FUTURES_WAIT_FOR_ALL_H

#include <futures/futures/traits/is_future.h>
#include <futures/algorithm/detail/traits/range/range/concepts.h>
#include <type_traits>

namespace futures {
    /** \addtogroup futures Futures
     *  @{
     */
    /** \addtogroup waiting Waiting
     *  @{
     */

    /// \brief Wait for a sequence of futures to be ready
    ///
    /// This function waits for all futures in the range [`first`, `last`) to be
    /// ready. It simply waits iteratively for each of the futures to be ready.
    ///
    /// \note This function is adapted from boost::wait_for_all
    ///
    /// \see
    /// [boost.thread
    /// wait_for_all](https://www.boost.org/doc/libs/1_78_0/doc/html/thread/synchronization.html#thread.synchronization.futures.reference.wait_for_all)
    ///
    /// \tparam Iterator Iterator type in a range of futures
    /// \param first Iterator to the first element in the range
    /// \param last Iterator to one past the last element in the range
    template <
        typename Iterator
#ifndef FUTURES_DOXYGEN
        ,
        typename std::
            enable_if_t<is_future_v<detail::iter_value_t<Iterator>>, int> = 0
#endif
        >
    void
    wait_for_all(Iterator first, Iterator last) {
        for (Iterator it = first; it != last; ++it) {
            it->wait();
        }
    }

    /// \brief Wait for a sequence of futures to be ready
    ///
    /// This function waits for all futures in the range `r` to be ready.
    /// It simply waits iteratively for each of the futures to be ready.
    ///
    /// \note This function is adapted from boost::wait_for_all
    ///
    /// \see
    /// [boost.thread
    /// wait_for_all](https://www.boost.org/doc/libs/1_78_0/doc/html/thread/synchronization.html#thread.synchronization.futures.reference.wait_for_all)
    ///
    /// \tparam Range A range of futures type
    /// \param r Range of futures
    template <
        typename Range
#ifndef FUTURES_DOXYGEN
        ,
        typename std::enable_if_t<
            detail::range<Range> && is_future_v<detail::range_value_t<Range>>,
            int> = 0
#endif
        >
    void
    wait_for_all(Range &&r) {
        wait_for_all(std::begin(r), std::end(r));
    }

    /// \brief Wait for a sequence of futures to be ready
    ///
    /// This function waits for all specified futures `fs`... to be ready.
    ///
    /// It creates a compile-time fixed-size data structure to store references
    /// to all of the futures and then waits for each of the futures to be
    /// ready.
    ///
    /// \note This function is adapted from boost::wait_for_all
    ///
    /// \see
    /// [boost.thread
    /// wait_for_all](https://www.boost.org/doc/libs/1_78_0/doc/html/thread/synchronization.html#thread.synchronization.futures.reference.wait_for_all)
    ///
    /// \tparam Fs A list of future types
    /// \param fs A list of future objects
    template <
        typename... Fs
#ifndef FUTURES_DOXYGEN
        ,
        typename std::enable_if_t<
            std::conjunction_v<is_future<std::decay_t<Fs>>...>,
            int> = 0
#endif
        >
    void
    wait_for_all(Fs &&...fs) {
        (fs.wait(), ...);
    }

    /** @} */
    /** @} */
} // namespace futures

#endif // FUTURES_WAIT_FOR_ALL_H
