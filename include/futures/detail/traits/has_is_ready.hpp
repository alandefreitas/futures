//
// Created by alandefreitas on 10/15/21.
//

#ifndef FUTURES_DETAIL_TRAITS_HAS_IS_READY_HPP
#define FUTURES_DETAIL_TRAITS_HAS_IS_READY_HPP

#include <type_traits>

namespace futures::detail {
    /** @addtogroup futures Futures
     *  @{
     */
    /** @addtogroup future-traits Future Traits
     *  @{
     */

    /// Check if a type implements the is_ready function and it returns bool
    /// This is what we use to identify the return type of a future type
    /// candidate However, this doesn't mean the type is a future in the terms
    /// of the is_future concept
    template <typename T, typename = void>
    struct has_is_ready : std::false_type
    {};

    template <typename T>
    struct has_is_ready<T, std::void_t<decltype(std::declval<T>().is_ready())>>
        : std::is_same<bool, decltype(std::declval<T>().is_ready())>
    {};

    template <typename T>
    constexpr bool has_is_ready_v = has_is_ready<T>::value;

    /** @} */
    /** @} */
} // namespace futures::detail

#endif // FUTURES_DETAIL_TRAITS_HAS_IS_READY_HPP
