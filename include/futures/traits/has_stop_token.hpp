//
// Copyright (c) 2021 alandefreitas (alandefreitas@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//

#ifndef FUTURES_TRAITS_HAS_STOP_TOKEN_HPP
#define FUTURES_TRAITS_HAS_STOP_TOKEN_HPP

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

    /// Customization point to define future having a common stop token
    template <typename>
    struct has_stop_token : std::false_type {};

    /// Customization point to define future having a common stop token
    template <class T>
    constexpr bool has_stop_token_v = has_stop_token<T>::value;

    /** @} */ // @addtogroup future-traits Future Traits
    /** @} */ // @addtogroup futures Futures
} // namespace futures

#endif // FUTURES_TRAITS_HAS_STOP_TOKEN_HPP
