//
// Copyright (c) 2022 alandefreitas (alandefreitas@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//

#ifndef FUTURES_ALGORITHM_TRAITS_IS_CONSTRUCTIBLE_FROM_HPP
#define FUTURES_ALGORITHM_TRAITS_IS_CONSTRUCTIBLE_FROM_HPP

#include <type_traits>

namespace futures {
    /** \addtogroup algorithms Algorithms
     *  @{
     */

    /** \addtogroup traits Traits
     *  @{
     */


    /** \brief A C++17 type trait equivalent to the C++20 constructible_from
     * concept
     */
#ifdef FUTURES_DOXYGEN
    template <class T, class... Args>
    using is_constructible_from = __see_below__;
#else
    template <class T, class... Args>
    struct is_constructible_from
        : std::conjunction<
              std::is_destructible<T>,
              std::is_constructible<T, Args...>>
    {};
#endif
    template <class T, class... Args>
    bool constexpr is_constructible_from_v = is_constructible_from<T>::value;
    /** @}*/
    /** @}*/

} // namespace futures

#endif // FUTURES_ALGORITHM_TRAITS_IS_CONSTRUCTIBLE_FROM_HPP
