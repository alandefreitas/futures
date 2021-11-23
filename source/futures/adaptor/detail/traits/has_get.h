//
// Created by alandefreitas on 10/15/21.
//

#ifndef FUTURES_HAS_GET_H
#define FUTURES_HAS_GET_H

namespace futures {
    /** \addtogroup futures Futures
     *  @{
     */
    /** \addtogroup future-traits Future Traits
     *  @{
     */

    namespace detail {
        /// Check if a type implements the get function
        /// This is what we use to identify the return type of a future type candidate
        /// However, this doesn't mean the type is a future in the terms of the is_future concept
        template <typename T, typename = void> struct has_get : std::false_type {};

        template <typename T> struct has_get<T, std::void_t<decltype(std::declval<T>().get())>> : std::true_type {};
    } // namespace detail
    /** @} */  // \addtogroup future-traits Future Traits
    /** @} */  // \addtogroup futures Futures
} // namespace futures

#endif // FUTURES_HAS_GET_H
