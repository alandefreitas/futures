//
// Copyright (c) 2022 alandefreitas (alandefreitas@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//

#ifndef FUTURES_ALGORITHM_TRAITS_IS_SEMIREGULAR_HPP
#define FUTURES_ALGORITHM_TRAITS_IS_SEMIREGULAR_HPP

#include <futures/algorithm/traits/is_copyable.hpp>
#include <futures/algorithm/traits/is_default_initializable.hpp>
#include <type_traits>

namespace futures {
    /** \addtogroup algorithms Algorithms
     *  @{
     */

    /** \addtogroup traits Traits
     *  @{
     */


    /** \brief A C++17 type trait equivalent to the C++20 semiregular
     * concept
     */
#ifdef FUTURES_DOXYGEN
    template <class T>
    using is_semiregular = __see_below__;
#else
    template <class T, class = void>
    struct is_semiregular : std::false_type
    {};

    template <class T>
    struct is_semiregular<
        T,
        std::enable_if_t<
            // clang-format off
            is_copyable_v<T> &&
            is_default_initializable_v<T>
            // clang-format on
            >> : std::true_type
    {};
#endif
    template <class T>
    bool constexpr is_semiregular_v = is_semiregular<T>::value;

    /** @}*/
    /** @}*/

} // namespace futures

#endif // FUTURES_ALGORITHM_TRAITS_IS_SEMIREGULAR_HPP
