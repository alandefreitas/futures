//
// Created by Alan Freitas on 8/16/21.
//

#ifndef FUTURES_FUTURE_RETURN_H
#define FUTURES_FUTURE_RETURN_H

#include "is_reference_wrapper.h"
#include <type_traits>

namespace futures {
    /** \addtogroup future Futures
     *  @{
     */
    /** \addtogroup future-traits Future Traits
     *  @{
     */

    /// Determine the type to be stored and returned by a future object
    template <class T>
    using future_return = std::conditional<is_reference_wrapper_v<std::decay_t<T>>, T &, std::decay_t<T>>;

    /// Determine the type to be stored and returned by a future object
    template <class T> using future_return_t = typename future_return<T>::type;

    /** @} */  // \addtogroup future-traits Future Traits
    /** @} */  // \addtogroup future Futures
} // namespace futures

#endif // FUTURES_FUTURE_RETURN_H
