//
// Copyright (c) alandefreitas 12/3/21.
// See accompanying file LICENSE
//

#ifndef FUTURES_TYPE_MEMBER_OR_VOID_H
#define FUTURES_TYPE_MEMBER_OR_VOID_H

#include <type_traits>

namespace futures::detail {
    /** \addtogroup futures Futures
     *  @{
     */

    /// \brief Return T::type or void as a placeholder if T::type doesn't exist
    /// This class is meant to avoid errors in std::conditional
    template <class, class = void> struct type_member_or_void { using type = void; };
    template <class T> struct type_member_or_void<T, std::void_t<typename T::type>> {
        using type = typename T::type;
    };
    template <class T> using type_member_or_void_t = typename type_member_or_void<T>::type;

    /** @} */
}

#endif // FUTURES_TYPE_MEMBER_OR_VOID_H
