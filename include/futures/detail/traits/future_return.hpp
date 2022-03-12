//
// Copyright (c) 2021 alandefreitas (alandefreitas@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//

#ifndef FUTURES_DETAIL_TRAITS_FUTURE_RETURN_HPP
#define FUTURES_DETAIL_TRAITS_FUTURE_RETURN_HPP

#include <futures/detail/traits/is_reference_wrapper.hpp>
#include <type_traits>

namespace futures {
    /** @addtogroup futures Futures
     *  @{
     */
    /** @addtogroup future-traits Future Traits
     *  @{
     */

    /// Determine the type to be stored and returned by a future object
    template <class T>
    struct future_return
        : std::conditional<
              detail::is_reference_wrapper_v<std::decay_t<T>>,
              T &,
              std::decay_t<T>>
    {};

    /// Determine the type to be stored and returned by a future object
    template <class T>
    using future_return_t = typename future_return<T>::type;

    /** @} */ // @addtogroup future-traits Future Traits
    /** @} */ // @addtogroup futures Futures
} // namespace futures

#endif // FUTURES_DETAIL_TRAITS_FUTURE_RETURN_HPP
