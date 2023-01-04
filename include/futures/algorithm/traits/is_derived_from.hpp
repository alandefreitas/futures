//
// Copyright (c) 2022 alandefreitas (alandefreitas@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//

#ifndef FUTURES_ALGORITHM_TRAITS_IS_DERIVED_FROM_HPP
#define FUTURES_ALGORITHM_TRAITS_IS_DERIVED_FROM_HPP

/**
 *  @file algorithm/traits/is_derived_from.hpp
 *  @brief `is_derived_from` trait
 *
 *  This file defines the `is_derived_from` trait.
 */

#include <futures/algorithm/traits/iter_reference.hpp>
#include <futures/algorithm/traits/iter_rvalue_reference.hpp>
#include <futures/algorithm/traits/iter_value.hpp>
#include <futures/detail/traits/std_type_traits.hpp>
#include <type_traits>

namespace futures {
    /** @addtogroup algorithms Algorithms
     *  @{
     */

    /** @addtogroup traits Traits
     *  @{
     */


    /// A type trait equivalent to the `derived_from` concept
    /**
     * @see [`std::derived_from`](https://en.cppreference.com/w/cpp/concepts/derived_from)
     */
#ifdef FUTURES_DOXYGEN
    template <class Derived, class Base>
    using is_derived_from = std::bool_constant<derived_from<Derived, Base>>;
#else
    template <class Derived, class Base>
    using is_derived_from = detail::conjunction<
        std::is_base_of<Base, Derived>,
        std::is_convertible<const volatile Derived*, const volatile Base*>>;
#endif

    /// @copydoc is_derived_from
    template <class Derived, class Base>
    constexpr bool is_derived_from_v = is_derived_from<Derived, Base>::value;

    /** @} */
    /** @} */

} // namespace futures

#endif // FUTURES_ALGORITHM_TRAITS_IS_DERIVED_FROM_HPP
