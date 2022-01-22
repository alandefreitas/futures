//
// Copyright (c) 2022 alandefreitas (alandefreitas@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//

#ifndef FUTURES_ALGORITHM_TRAITS_REMOVE_CVREF_H
#define FUTURES_ALGORITHM_TRAITS_REMOVE_CVREF_H

#include <type_traits>

namespace futures {
    /** A C++17 type trait equivalent to the C++20 remove_cvref
     * concept
     */
    template <class T>
    struct remove_cvref
    {
        using type = std::remove_cv_t<std::remove_reference_t<T>>;
    };

    template <class T>
    using remove_cvref_t = typename remove_cvref<T>::type;


} // namespace futures

#endif // FUTURES_ALGORITHM_TRAITS_REMOVE_CVREF_H
