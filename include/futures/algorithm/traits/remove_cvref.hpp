//
// Copyright (c) 2022 alandefreitas (alandefreitas@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//

#ifndef FUTURES_ALGORITHM_TRAITS_REMOVE_CVREF_HPP
#define FUTURES_ALGORITHM_TRAITS_REMOVE_CVREF_HPP

/**
 *  @file algorithm/traits/remove_cvref.hpp
 *  @brief `remove_cvref` trait
 *
 *  This file defines the `remove_cvref` trait.
 */

#include <futures/detail/deps/boost/mp11/utility.hpp>
#include <type_traits>

namespace futures {
    /** @addtogroup algorithms Algorithms
     *  @{
     */

    /** @addtogroup traits Traits
     *  @{
     */

    /// @brief A type trait equivalent to `std::remove_cvref`
    /**
     * @see [`std::remove_cvref`](https://en.cppreference.com/w/cpp/types/remove_cvref)
     */
#ifdef FUTURES_DOXYGEN
    template <class T>
    using remove_cvref = std::remove_cvref<T>;
#else
    template <class T>
    using remove_cvref = detail::mp_identity<
        std::remove_cv_t<std::remove_reference_t<T>>>;
#endif

    /// @copydoc remove_cvref
    template <class T>
    using remove_cvref_t = typename remove_cvref<T>::type;

    /** @} */
    /** @} */
} // namespace futures

#endif // FUTURES_ALGORITHM_TRAITS_REMOVE_CVREF_HPP
