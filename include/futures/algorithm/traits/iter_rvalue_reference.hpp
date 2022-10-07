//
// Copyright (c) 2022 alandefreitas (alandefreitas@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//

#ifndef FUTURES_ALGORITHM_TRAITS_ITER_RVALUE_REFERENCE_HPP
#define FUTURES_ALGORITHM_TRAITS_ITER_RVALUE_REFERENCE_HPP

#include <futures/algorithm/traits/iter_value.hpp>
#include <iterator>
#include <type_traits>

namespace futures {
    /** @addtogroup algorithms Algorithms
     *  @{
     */

    /** @addtogroup traits Traits
     *  @{
     */


    /** \brief A C++17 type trait equivalent to the C++20 iter_rvalue_reference
     * concept
     */
#ifdef FUTURES_DOXYGEN
    template <class T>
    using iter_rvalue_reference = __see_below__;
#else
    template <class T, class = void>
    struct iter_rvalue_reference {};

    template <class T>
    struct iter_rvalue_reference<T, std::void_t<iter_value_t<T>>> {
        using type = std::add_rvalue_reference<iter_value_t<T>>;
    };
#endif
    template <class T>
    using iter_rvalue_reference_t = typename iter_rvalue_reference<T>::type;

    /** @}*/
    /** @}*/

} // namespace futures

#endif // FUTURES_ALGORITHM_TRAITS_ITER_RVALUE_REFERENCE_HPP
