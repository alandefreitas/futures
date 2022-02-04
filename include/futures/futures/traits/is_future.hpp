//
// Copyright (c) 2021 alandefreitas (alandefreitas@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//

#ifndef FUTURES_FUTURES_TRAITS_IS_FUTURE_HPP
#define FUTURES_FUTURES_TRAITS_IS_FUTURE_HPP

#include <future>
#include <type_traits>

namespace futures {
    /** \addtogroup futures Futures
     *  @{
     */
    /** \addtogroup future-traits Future Traits
     *
     * \brief Determine properties of future types
     *
     *  @{
     */

    /// \brief Customization point to determine if a type is a future type
    template <typename>
    struct is_future : std::false_type
    {};

    /// \brief Customization point to determine if a type is a future type
    /// (specialization for std::future<T>)
    template <typename T>
    struct is_future<std::future<T>> : std::true_type
    {};

    /// \brief Customization point to determine if a type is a future type
    /// (specialization for std::shared_future<T>)
    template <typename T>
    struct is_future<std::shared_future<T>> : std::true_type
    {};

    /// \brief Customization point to determine if a type is a future type as a
    /// bool value
    template <class T>
    constexpr bool is_future_v = is_future<T>::value;

    /// \brief Customization point to determine if a type is a shared future type
    template <typename>
    struct has_ready_notifier : std::false_type
    {};

    /// \brief Customization point to determine if a type is a shared future type
    template <class T>
    constexpr bool has_ready_notifier_v = has_ready_notifier<T>::value;

    /// \brief Customization point to determine if a type is a shared future type
    template <typename>
    struct is_shared_future : std::false_type
    {};

    /// \brief Customization point to determine if a type is a shared future
    /// type (specialization for std::shared_future<T>)
    template <typename T>
    struct is_shared_future<std::shared_future<T>> : std::true_type
    {};

    /// \brief Customization point to determine if a type is a shared future type
    template <class T>
    constexpr bool is_shared_future_v = is_shared_future<T>::value;

    /// \brief Customization point to define future as supporting lazy
    /// continuations
    template <typename>
    struct is_continuable : std::false_type
    {};

    /// \brief Customization point to define future as supporting lazy
    /// continuations
    template <class T>
    constexpr bool is_continuable_v = is_continuable<T>::value;

    /// \brief Customization point to define future as stoppable
    template <typename>
    struct is_stoppable : std::false_type
    {};

    /// \brief Customization point to define future as stoppable
    template <class T>
    constexpr bool is_stoppable_v = is_stoppable<T>::value;

    /// \brief Customization point to define future having a common stop token
    template <typename>
    struct has_stop_token : std::false_type
    {};

    /// \brief Customization point to define future having a common stop token
    template <class T>
    constexpr bool has_stop_token_v = has_stop_token<T>::value;

    /// \brief Customization point to define future as deferred
    template <typename>
    struct is_deferred : std::false_type
    {};

    /// \brief Customization point to define future as deferred
    template <class T>
    constexpr bool is_deferred_v = is_deferred<T>::value;

    /** @} */ // \addtogroup future-traits Future Traits
    /** @} */ // \addtogroup futures Futures
} // namespace futures

#endif // FUTURES_FUTURES_TRAITS_IS_FUTURE_HPP
