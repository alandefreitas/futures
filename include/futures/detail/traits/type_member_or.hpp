//
// Copyright (c) 2021 alandefreitas (alandefreitas@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//

#ifndef FUTURES_DETAIL_TRAITS_TYPE_MEMBER_OR_HPP
#define FUTURES_DETAIL_TRAITS_TYPE_MEMBER_OR_HPP

#include <type_traits>

namespace futures::detail {
    /** \addtogroup futures Futures
     *  @{
     */

    /// \brief Return T::type or a second type as a placeholder if T::type
    /// doesn't exist This class is meant to avoid errors in std::conditional
    template <class, class Placeholder = void, class = void>
    struct type_member_or
    {
        using type = Placeholder;
    };

    template <class T, class Placeholder>
    struct type_member_or<T, Placeholder, std::void_t<typename T::type>>
    {
        using type = typename T::type;
    };

    template <class T, class Placeholder>
    using type_member_or_t = typename type_member_or<T>::type;

    /** @} */
} // namespace futures::detail

#endif // FUTURES_DETAIL_TRAITS_TYPE_MEMBER_OR_HPP
