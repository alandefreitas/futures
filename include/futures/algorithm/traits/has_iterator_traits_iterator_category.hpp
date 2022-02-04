//
// Copyright (c) 2022 alandefreitas (alandefreitas@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//

#ifndef FUTURES_ALGORITHM_TRAITS_HAS_ITERATOR_TRAITS_ITERATOR_CATEGORY_HPP
#define FUTURES_ALGORITHM_TRAITS_HAS_ITERATOR_TRAITS_ITERATOR_CATEGORY_HPP

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
     * "has-iterator-traits-iterator-category" concept
     */
#ifdef FUTURES_DOXYGEN
    template <class T>
    using has_iterator_traits_iterator_category = __see_below__;
#else
    template <class T, class = void>
    struct has_iterator_traits_iterator_category : std::false_type
    {};

    template <class T>
    struct has_iterator_traits_iterator_category<
        T,
        std::void_t<typename std::iterator_traits<T>::iterator_category>>
        : std::true_type
    {};
#endif
    template <class T>
    bool constexpr has_iterator_traits_iterator_category_v
        = has_iterator_traits_iterator_category<T>::value;
    /** @}*/
    /** @}*/

} // namespace futures

#endif // FUTURES_ALGORITHM_TRAITS_HAS_ITERATOR_TRAITS_ITERATOR_CATEGORY_HPP
