//
// Copyright (c) 2021 alandefreitas (alandefreitas@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//

#ifndef FUTURES_DETAIL_TRAITS_TYPE_MEMBER_OR_VOID_HPP
#define FUTURES_DETAIL_TRAITS_TYPE_MEMBER_OR_VOID_HPP

#include <type_traits>

namespace futures::detail {
    /** \addtogroup futures Futures
     *  @{
     */

    /// \brief Return T::type or void as a placeholder if T::type doesn't exist
    /// This class is meant to avoid errors in std::conditional
    template <class, class = void>
    struct type_member_or_void
    {
        using type = void;
    };

    template <class T>
    struct type_member_or_void<T, std::void_t<typename T::type>>
    {
        using type = typename T::type;
    };

    template <class T>
    using type_member_or_void_t = typename type_member_or_void<T>::type;

    /** @} */
} // namespace futures::detail

#endif // FUTURES_DETAIL_TRAITS_TYPE_MEMBER_OR_VOID_HPP
