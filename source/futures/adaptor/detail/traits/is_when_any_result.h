//
// Copyright (c) alandefreitas 12/4/21.
// See accompanying file LICENSE
//

#ifndef FUTURES_IS_WHEN_ANY_RESULT_H
#define FUTURES_IS_WHEN_ANY_RESULT_H

#include <futures/adaptor/when_any_result.h>
#include <type_traits>

namespace futures::detail {
    /** \addtogroup futures Futures
     *  @{
     */

    /// Check if type is a when_any_result
    template <typename> struct is_when_any_result : std::false_type {};
    template <typename Sequence> struct is_when_any_result<when_any_result<Sequence>> : std::true_type {};
    template <typename Sequence> struct is_when_any_result<const when_any_result<Sequence>> : std::true_type {};
    template <typename Sequence> struct is_when_any_result<when_any_result<Sequence> &> : std::true_type {};
    template <typename Sequence> struct is_when_any_result<when_any_result<Sequence> &&> : std::true_type {};
    template <typename Sequence> struct is_when_any_result<const when_any_result<Sequence> &> : std::true_type {};
    template <class T> constexpr bool is_when_any_result_v = is_when_any_result<T>::value;

    /** @} */
}


#endif // FUTURES_IS_WHEN_ANY_RESULT_H
