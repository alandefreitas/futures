//
// Created by alandefreitas on 10/15/21.
//

#ifndef FUTURES_DETAIL_TRAITS_HAS_SHARE_HPP
#define FUTURES_DETAIL_TRAITS_HAS_SHARE_HPP

#include <type_traits>

namespace futures {
    /** @addtogroup futures Futures
     *  @{
     */
    /** @addtogroup future-traits Future Traits
     *  @{
     */

    namespace detail {
        /// Check if a type implements the get function
        /// This is what we use to identify the return type of a future type
        /// candidate However, this doesn't mean the type is a future in the
        /// terms of the is_future concept
        template <typename T, typename = void>
        struct has_share : std::false_type {};

        template <typename T>
        struct has_share<T, std::void_t<decltype(std::declval<T>().share())>>
            : std::true_type {};
    }         // namespace detail
    /** @} */ // @addtogroup future-traits Future Traits
    /** @} */ // @addtogroup futures Futures
} // namespace futures

#endif // FUTURES_DETAIL_TRAITS_HAS_SHARE_HPP
