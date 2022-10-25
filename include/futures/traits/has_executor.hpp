//
// Copyright (c) 2021 alandefreitas (alandefreitas@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//

#ifndef FUTURES_TRAITS_HAS_EXECUTOR_HPP
#define FUTURES_TRAITS_HAS_EXECUTOR_HPP

/**
 *  @file traits/has_executor.hpp
 *  @brief `has_executor` trait
 *
 *  This file defines the `has_executor` trait.
 */

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

    /// Determine if a future type has an executor
    template <typename>
    struct has_executor : std::false_type {};

    /// @copydoc has_executor
    template <class T>
    constexpr bool has_executor_v = has_executor<T>::value;

    /** @} */ // @addtogroup future-traits Future Traits
    /** @} */ // @addtogroup futures Futures
} // namespace futures

#endif // FUTURES_TRAITS_HAS_EXECUTOR_HPP
