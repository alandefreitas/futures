//
// Copyright (c) 2022 alandefreitas (alandefreitas@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//

#ifndef FUTURES_ALGORITHM_TRAITS_HAS_DIFFERENCE_TYPE_HPP
#define FUTURES_ALGORITHM_TRAITS_HAS_DIFFERENCE_TYPE_HPP

#include <type_traits>

namespace futures {
    /** @addtogroup algorithms Algorithms
     *  @{
     */

    /** @addtogroup traits Traits
     *  @{
     */

    /** \brief A C++17 type trait equivalent to the C++20 has-member-difference-type
     * concept
     */
#ifdef FUTURES_DOXYGEN
    template <class T>
    using has_difference_type = __see_below__;
#else
    template <class T, class = void>
    struct has_difference_type : std::false_type
    {};

    template <class T>
    struct has_difference_type<T, std::void_t<typename T::value_type>>
        : std::true_type
    {};
#endif
    template <class T>
    bool constexpr has_difference_type_v = has_difference_type<T>::value;
    /** @}*/
    /** @}*/
} // namespace futures

#endif // FUTURES_ALGORITHM_TRAITS_HAS_DIFFERENCE_TYPE_HPP
