//
// Copyright (c) 2022 alandefreitas (alandefreitas@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//

#ifndef FUTURES_ALGORITHM_TRAITS_RANGE_VALUE_HPP
#define FUTURES_ALGORITHM_TRAITS_RANGE_VALUE_HPP

#include <futures/algorithm/traits/is_range.hpp>
#include <futures/algorithm/traits/iter_value.hpp>
#include <futures/algorithm/traits/iterator.hpp>
#include <iterator>
#include <type_traits>

namespace futures {
    /** @addtogroup algorithms Algorithms
     *  @{
     */

    /** @addtogroup traits Traits
     *  @{
     */


    /** \brief A C++17 type trait equivalent to the C++20 range_value
     * concept
     */
#ifdef FUTURES_DOXYGEN
    template <class R>
    using range_value = __see_below__;
#else
    template <class R, class = void>
    struct range_value
    {};

    template <class R>
    struct range_value<R, std::enable_if_t<is_range_v<R>>>
    {
        using type = iter_value_t<iterator_t<R>>;
    };
#endif
    template <class R>
    using range_value_t = typename range_value<R>::type;

    /** @}*/
    /** @}*/

} // namespace futures

#endif // FUTURES_ALGORITHM_TRAITS_RANGE_VALUE_HPP
