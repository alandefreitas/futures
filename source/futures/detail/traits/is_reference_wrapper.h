//
// Created by Alan Freitas on 8/16/21.
//

#ifndef FUTURES_IS_REFERENCE_WRAPPER_H
#define FUTURES_IS_REFERENCE_WRAPPER_H

#include <type_traits>

namespace futures {
    /// Check if type is a reference_wrapper
    template <typename> struct is_reference_wrapper : std::false_type {};

    template <class T> struct is_reference_wrapper<std::reference_wrapper<T>> : std::true_type {};

    template <class T> constexpr bool is_reference_wrapper_v = is_reference_wrapper<T>::value;
} // namespace futures

#endif // FUTURES_IS_REFERENCE_WRAPPER_H
