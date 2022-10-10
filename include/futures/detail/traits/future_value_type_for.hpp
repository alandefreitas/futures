//
// Copyright (c) 2021 alandefreitas (alandefreitas@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//

#ifndef FUTURES_DETAIL_TRAITS_FUTURE_VALUE_TYPE_FOR_HPP
#define FUTURES_DETAIL_TRAITS_FUTURE_VALUE_TYPE_FOR_HPP

#include <futures/detail/traits/is_reference_wrapper.hpp>
#include <type_traits>

namespace futures::detail {
    // Check if type is a reference_wrapper
    template <typename>
    struct is_reference_wrapper : std::false_type {};

    template <class T>
    struct is_reference_wrapper<std::reference_wrapper<T>> : std::true_type {};

    template <class T>
    constexpr bool is_reference_wrapper_v = is_reference_wrapper<T>::value;

    // Determine the type to be stored and returned by a future object
    /*
     * Given a task that returns `T`, this trait determines the value type `R`
     * of the corresponding `future<R>` for the task.
     */
    template <class T>
    struct future_value_type_for
        : std::conditional<
              detail::is_reference_wrapper_v<std::decay_t<T>>,
              T &,
              std::decay_t<T>> {};

    /// @copydoc
    template <class T>
    using future_value_type_for_t = typename future_value_type_for<T>::type;
} // namespace futures::detail

#endif // FUTURES_DETAIL_TRAITS_FUTURE_VALUE_TYPE_FOR_HPP
