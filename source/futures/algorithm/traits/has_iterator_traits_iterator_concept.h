//
// Copyright (c) 2022 alandefreitas (alandefreitas@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//

#ifndef FUTURES_ALGORITHM_TRAITS_HAS_ITERATOR_TRAITS_ITERATOR_CONCEPT_H
#define FUTURES_ALGORITHM_TRAITS_HAS_ITERATOR_TRAITS_ITERATOR_CONCEPT_H

#include <type_traits>
#include <iterator>

namespace futures {
    /** \addtogroup algorithms Algorithms
     *  @{
     */

    /** \addtogroup traits Traits
     *  @{
     */


    /** \brief A C++17 type trait equivalent to the C++20
     * "has-iterator-traits-iterator-concept" concept
     */
#ifdef FUTURES_DOXYGEN
    template <class T>
    using has_iterator_traits_iterator_concept = __see_below__;
#else
    template <class T, class = void>
    struct has_iterator_traits_iterator_concept : std::false_type
    {};

    template <class T>
    struct has_iterator_traits_iterator_concept<
        T,
        std::void_t<typename std::iterator_traits<T>::iterator_concept>>
        : std::true_type
    {};
#endif
    template <class T>
    bool constexpr has_iterator_traits_iterator_concept_v
        = has_iterator_traits_iterator_concept<T>::value;
    /** @}*/
    /** @}*/

} // namespace futures

#endif // FUTURES_ALGORITHM_TRAITS_HAS_ITERATOR_TRAITS_ITERATOR_CONCEPT_H
