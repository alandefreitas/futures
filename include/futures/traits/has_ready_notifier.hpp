//
// Copyright (c) 2021 alandefreitas (alandefreitas@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//

#ifndef FUTURES_TRAITS_HAS_READY_NOTIFIER_HPP
#define FUTURES_TRAITS_HAS_READY_NOTIFIER_HPP

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

    /// Customization point to determine if a type is a shared future type
    template <typename>
    struct has_ready_notifier : std::false_type {};

    /// Customization point to determine if a type is a shared future type
    template <class T>
    constexpr bool has_ready_notifier_v = has_ready_notifier<T>::value;

    /** @} */ // @addtogroup future-traits Future Traits
    /** @} */ // @addtogroup futures Futures
} // namespace futures

#endif // FUTURES_TRAITS_HAS_READY_NOTIFIER_HPP
