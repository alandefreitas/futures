//
// Created by Alan Freitas on 8/19/21.
//

#ifndef CPP_MANIFEST_IS_TUPLE_H
#define CPP_MANIFEST_IS_TUPLE_H

#include <type_traits>
#include <tuple>

namespace futures {
    /// Check if type is a tuple
    template <typename> struct is_tuple : std::false_type {};

    template <typename... Args> struct is_tuple<std::tuple<Args...>> : std::true_type {};
    template <typename... Args> struct is_tuple<const std::tuple<Args...>> : std::true_type {};
    template <typename... Args> struct is_tuple<std::tuple<Args...> &> : std::true_type {};
    template <typename... Args> struct is_tuple<std::tuple<Args...> &&> : std::true_type {};
    template <typename... Args> struct is_tuple<const std::tuple<Args...> &> : std::true_type {};

    template <class T> constexpr bool is_tuple_v = is_tuple<T>::value;
} // namespace futures

#endif // CPP_MANIFEST_IS_TUPLE_H
