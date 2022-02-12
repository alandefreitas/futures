//
// Copyright (c) 2022 alandefreitas (alandefreitas@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//

#ifndef FUTURES_ALGORITHM_TRAITS_IS_CONVERTIBLE_TO_HPP
#define FUTURES_ALGORITHM_TRAITS_IS_CONVERTIBLE_TO_HPP

#include <type_traits>

namespace futures {
    /** @addtogroup algorithms Algorithms
     *  @{
     */

    /** @addtogroup traits Traits
     *  @{
     */


    /** \brief A C++17 type trait equivalent to the C++20 convertible_to
     * concept
     */
#ifdef FUTURES_DOXYGEN
    template <class From, class To>
    using is_convertible_to = __see_below__;
#else
    template <class From, class To, class = void>
    struct is_convertible_to : std::false_type
    {};

    template <class From, class To>
    struct is_convertible_to<
        From, To,
        std::void_t<
            // clang-format off
            std::enable_if_t<std::is_convertible_v<From, To>>,
            decltype(static_cast<To>(std::declval<From>()))
            // clang-format on
            >> : std::true_type
    {};
#endif
    template <class From, class To>
    bool constexpr is_convertible_to_v = is_convertible_to<From, To>::value;
    /** @}*/
    /** @}*/

} // namespace futures

#endif // FUTURES_ALGORITHM_TRAITS_IS_CONVERTIBLE_TO_HPP
