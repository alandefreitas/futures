//
// Copyright (c) 2022 alandefreitas (alandefreitas@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//

#ifndef FUTURES_ALGORITHM_TRAITS_HAS_ITERATOR_TRAITS_VALUE_TYPE_HPP
#define FUTURES_ALGORITHM_TRAITS_HAS_ITERATOR_TRAITS_VALUE_TYPE_HPP

#include <type_traits>
#include <iterator>

namespace futures {
    /** @addtogroup algorithms Algorithms
     *  @{
     */

    /** @addtogroup traits Traits
     *  @{
     */


    /** \brief A C++17 type trait equivalent to the C++20
     * "has-iterator-traits-value-type" concept
     */
#ifdef FUTURES_DOXYGEN
    template <class T>
    using has_iterator_traits_value_type = __see_below__;
#else
    template <class T, class = void>
    struct has_iterator_traits_value_type : std::false_type
    {};

    template <class T>
    struct has_iterator_traits_value_type<
        T,
        std::void_t<typename std::iterator_traits<T>::value_type>>
        : std::true_type
    {};
#endif
    template <class T>
    bool constexpr has_iterator_traits_value_type_v
        = has_iterator_traits_value_type<T>::value;
    /** @}*/
    /** @}*/

} // namespace futures

#endif // FUTURES_ALGORITHM_TRAITS_HAS_ITERATOR_TRAITS_VALUE_TYPE_HPP
