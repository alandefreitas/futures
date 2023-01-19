//
// Copyright (c) 2022 alandefreitas (alandefreitas@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//

#ifndef FUTURES_ALGORITHM_TRAITS_RANGE_VALUE_HPP
#define FUTURES_ALGORITHM_TRAITS_RANGE_VALUE_HPP

/**
 *  @file algorithm/traits/range_value.hpp
 *  @brief `range_value` trait
 *
 *  This file defines the `range_value` trait.
 */

#include <futures/algorithm/traits/is_range.hpp>
#include <futures/algorithm/traits/iter_value.hpp>
#include <futures/algorithm/traits/iterator.hpp>
#include <futures/detail/deps/boost/core/empty_value.hpp>
#include <futures/detail/deps/boost/mp11/utility.hpp>
#include <iterator>
#include <type_traits>

namespace futures {
    /** @addtogroup algorithms Algorithms
     *  @{
     */

    /** @addtogroup traits Traits
     *  @{
     */

    /// @brief A type trait equivalent to `std::range_value`
    /**
     * @see
     * [`std::ranges::iterator_t`](https://en.cppreference.com/w/cpp/ranges/iterator_t)
     */
#ifdef FUTURES_DOXYGEN
    template <class R>
    using range_value = std::range_value<R>;
#else
    template <class R, class = void>
    struct range_value {};

    template <class R>
    struct range_value<R, std::enable_if_t<is_range_v<R>>>
        : detail::mp_cond<
              is_range<R>,
              detail::mp_defer<
                  detail::mp_invoke_q,
                  detail::mp_compose<iterator_t, iter_value_t>,
                  R>> {};
#endif

    /// @copydoc range_value
    template <class R>
    using range_value_t = typename range_value<R>::type;

    /** @} */
    /** @} */

} // namespace futures

#endif // FUTURES_ALGORITHM_TRAITS_RANGE_VALUE_HPP
