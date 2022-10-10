//
// Copyright (c) 2021 alandefreitas (alandefreitas@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//

#ifndef FUTURES_TRAITS_IS_STOPPABLE_HPP
#define FUTURES_TRAITS_IS_STOPPABLE_HPP

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

    /// Customization point to define future as stoppable
    template <typename>
    struct is_stoppable : std::false_type {};

    /// Customization point to define future as stoppable
    template <class T>
    constexpr bool is_stoppable_v = is_stoppable<T>::value;

    /** @} */ // @addtogroup future-traits Future Traits
    /** @} */ // @addtogroup futures Futures
} // namespace futures

#endif // FUTURES_TRAITS_IS_STOPPABLE_HPP
