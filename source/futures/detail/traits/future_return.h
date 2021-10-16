//
// Created by Alan Freitas on 8/16/21.
//

#ifndef CPP_MANIFEST_FUTURE_RETURN_H
#define CPP_MANIFEST_FUTURE_RETURN_H

#include "is_reference_wrapper.h"
#include <type_traits>

namespace futures {
    /// Determine the type to be stored and returned by a future object
    template <class T>
    using future_return = std::conditional<is_reference_wrapper_v<std::decay_t<T>>, T &, std::decay_t<T>>;

    /// Determine the type to be stored and returned by a future object
    template <class T> using future_return_t = typename future_return<T>::type;

} // namespace futures

#endif // CPP_MANIFEST_FUTURE_RETURN_H
