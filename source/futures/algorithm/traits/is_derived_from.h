//
// Copyright (c) 2022 alandefreitas (alandefreitas@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//

#ifndef FUTURES_ALGORITHM_TRAITS_IS_DERIVED_FROM_H
#define FUTURES_ALGORITHM_TRAITS_IS_DERIVED_FROM_H

#include <futures/algorithm/traits/iter_reference.h>
#include <futures/algorithm/traits/iter_rvalue_reference.h>
#include <futures/algorithm/traits/iter_value.h>
#include <type_traits>

namespace futures {
    /** \addtogroup algorithms Algorithms
     *  @{
     */

    /** \addtogroup traits Traits
     *  @{
     */


    /** \brief A C++17 type trait equivalent to the C++20 derived_from concept
     */
#ifdef FUTURES_DOXYGEN
    template <class T>
    using is_derived_from = __see_below__;
#else
    template <class Derived, class Base>
    struct is_derived_from
        : std::conjunction<
              std::is_base_of<Base, Derived>,
              std::is_convertible<const volatile Derived*, const volatile Base*>>
    {};
#endif
    template <class Derived, class Base>
    bool constexpr is_derived_from_v = is_derived_from<Derived, Base>::value;
    /** @}*/
    /** @}*/

} // namespace futures

#endif // FUTURES_ALGORITHM_TRAITS_IS_DERIVED_FROM_H
