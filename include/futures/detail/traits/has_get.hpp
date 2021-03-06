//
// Created by alandefreitas on 10/15/21.
//

#ifndef FUTURES_DETAIL_TRAITS_HAS_GET_HPP
#define FUTURES_DETAIL_TRAITS_HAS_GET_HPP

namespace futures::detail {
    /** @addtogroup futures Futures
     *  @{
     */
    /** @addtogroup future-traits Future Traits
     *  @{
     */

    /// Check if a type implements the get function
    /**
     * This is what we use to identify the return type of a future type
     * candidate However, this doesn't mean the type is a future in the
     * terms of the is_future concept
     */
    template <typename T, typename = void>
    struct has_get : std::false_type
    {};

    template <typename T>
    struct has_get<T, std::void_t<decltype(std::declval<T>().get())>>
        : std::true_type
    {};

    /** @} */ // @addtogroup future-traits Future Traits
    /** @} */ // @addtogroup futures Futures
} // namespace futures::detail

#endif // FUTURES_DETAIL_TRAITS_HAS_GET_HPP
