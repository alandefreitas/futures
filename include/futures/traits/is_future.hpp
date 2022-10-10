//
// Copyright (c) 2021 alandefreitas (alandefreitas@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//

#ifndef FUTURES_TRAITS_IS_FUTURE_HPP
#define FUTURES_TRAITS_IS_FUTURE_HPP

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

    /// Customization point to determine if a type is a future type
    template <typename>
    struct is_future : std::false_type {};

    /// Customization point to determine if a type is a future type
    /// (specialization for std::future<T>)
    template <typename T>
    struct is_future<std::future<T>> : std::true_type {};

    /// Customization point to determine if a type is a future type
    /// (specialization for std::shared_future<T>)
    template <typename T>
    struct is_future<std::shared_future<T>> : std::true_type {};

    /// Customization point to determine if a type is a future type as a
    /// bool value
    template <class T>
    constexpr bool is_future_v = is_future<T>::value;

    /** @} */ // @addtogroup future-traits Future Traits
    /** @} */ // @addtogroup futures Futures
} // namespace futures

#endif // FUTURES_TRAITS_IS_FUTURE_HPP
