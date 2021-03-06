//
// Copyright (c) 2022 alandefreitas (alandefreitas@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//

#ifndef FUTURES_ALGORITHM_TRAITS_IS_INDIRECTLY_READABLE_HPP
#define FUTURES_ALGORITHM_TRAITS_IS_INDIRECTLY_READABLE_HPP

#include <futures/algorithm/traits/iter_reference.hpp>
#include <futures/algorithm/traits/iter_rvalue_reference.hpp>
#include <futures/algorithm/traits/iter_value.hpp>
#include <type_traits>

namespace futures {
    /** @addtogroup algorithms Algorithms
     *  @{
     */

    /** @addtogroup traits Traits
     *  @{
     */


    /** \brief A C++17 type trait equivalent to the C++20 indirectly_readable
     * concept
     */
#ifdef FUTURES_DOXYGEN
    template <class T>
    using is_indirectly_readable = __see_below__;
#else
    template <class T, class = void>
    struct is_indirectly_readable : std::false_type
    {};

    template <class T>
    struct is_indirectly_readable<
        T,
        std::void_t<
            iter_value_t<T>,
            iter_reference_t<T>,
            iter_rvalue_reference_t<T>,
            decltype(*std::declval<T>())>> : std::true_type
    {};
#endif
    template <class T>
    bool constexpr is_indirectly_readable_v = is_indirectly_readable<T>::value;
    /** @}*/
    /** @}*/

} // namespace futures

#endif // FUTURES_ALGORITHM_TRAITS_IS_INDIRECTLY_READABLE_HPP
