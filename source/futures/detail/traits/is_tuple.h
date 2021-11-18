//
// Created by Alan Freitas on 8/19/21.
//

#ifndef FUTURES_IS_TUPLE_H
#define FUTURES_IS_TUPLE_H

#include <type_traits>
#include <tuple>

namespace futures {
    /** \addtogroup Futures
     *  @{
     */
    /** \addtogroup future-traits Future Traits
     *  @{
     */

    /// Check if type is a tuple
    template <typename> struct is_tuple : std::false_type {};

    template <typename... Args> struct is_tuple<std::tuple<Args...>> : std::true_type {};
    template <typename... Args> struct is_tuple<const std::tuple<Args...>> : std::true_type {};
    template <typename... Args> struct is_tuple<std::tuple<Args...> &> : std::true_type {};
    template <typename... Args> struct is_tuple<std::tuple<Args...> &&> : std::true_type {};
    template <typename... Args> struct is_tuple<const std::tuple<Args...> &> : std::true_type {};

    template <class T> constexpr bool is_tuple_v = is_tuple<T>::value;
    /** @} */  // \addtogroup future-traits Future Traits
    /** @} */  // \addtogroup Futures
} // namespace futures

#endif // FUTURES_IS_TUPLE_H
