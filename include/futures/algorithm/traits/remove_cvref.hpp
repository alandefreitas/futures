//
// Copyright (c) 2022 alandefreitas (alandefreitas@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//

#ifndef FUTURES_ALGORITHM_TRAITS_REMOVE_CVREF_HPP
#define FUTURES_ALGORITHM_TRAITS_REMOVE_CVREF_HPP

#include <type_traits>

namespace futures {
    /** @addtogroup algorithms Algorithms
     *  @{
     */

    /** @addtogroup traits Traits
     *  @{
     */
    /** \brief A C++17 type trait equivalent to the C++20 remove_cvref
     * concept
     */
#ifdef FUTURES_DOXYGEN
    template <class T>
    using remove_cvref = __see_below__;
#else
    template <class T>
    struct remove_cvref {
        using type = std::remove_cv_t<std::remove_reference_t<T>>;
    };
#endif

    template <class T>
    using remove_cvref_t = typename remove_cvref<T>::type;

    /** @}*/
    /** @}*/

} // namespace futures

#endif // FUTURES_ALGORITHM_TRAITS_REMOVE_CVREF_HPP
