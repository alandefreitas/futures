//
// Copyright (c) alandefreitas 12/5/21.
// See accompanying file LICENSE
//

#ifndef FUTURES_TYPE_MEMBER_OR_H
#define FUTURES_TYPE_MEMBER_OR_H

#include <type_traits>

namespace futures::detail {
    /** \addtogroup futures Futures
     *  @{
     */

    /// \brief Return T::type or a second type as a placeholder if T::type doesn't exist
    /// This class is meant to avoid errors in std::conditional
    template <class, class Placeholder = void, class = void> struct type_member_or { using type = Placeholder; };

    template <class T, class Placeholder> struct type_member_or<T, Placeholder, std::void_t<typename T::type>> {
        using type = typename T::type;
    };

    template <class T, class Placeholder> using type_member_or_t = typename type_member_or<T>::type;

    /** @} */
} // namespace futures::detail

#endif // FUTURES_TYPE_MEMBER_OR_H
