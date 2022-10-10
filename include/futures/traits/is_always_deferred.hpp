//
// Copyright (c) 2021 alandefreitas (alandefreitas@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//

#ifndef FUTURES_TRAITS_IS_ALWAYS_DEFERRED_HPP
#define FUTURES_TRAITS_IS_ALWAYS_DEFERRED_HPP

#include <future>
#include <type_traits>

namespace futures {
    /** @addtogroup futures Futures
     *  @{
     */
    /** @addtogroup future-traits Future Traits
     *
     * \brief Determine properties of future types
     *
     *  @{
     */

    /// Customization point to define future as always deferred
    /**
     * Deferred futures allow some optimization that make it worth indicating
     * at compile time whether they can be applied.
     */
    template <typename>
    struct is_always_deferred : std::false_type {};

    /// Customization point to define future as always deferred
    template <class T>
    constexpr bool is_always_deferred_v = is_always_deferred<T>::value;

    /** @} */ // @addtogroup future-traits Future Traits
    /** @} */ // @addtogroup futures Futures
} // namespace futures

#endif // FUTURES_TRAITS_IS_ALWAYS_DEFERRED_HPP
