//
// Copyright (c) 2022 alandefreitas (alandefreitas@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//

#ifndef FUTURES_DETAIL_TRAITS_UNWRAP_REFWRAPPER_HPP
#define FUTURES_DETAIL_TRAITS_UNWRAP_REFWRAPPER_HPP

#include <functional>

namespace futures::detail {
    template <class T>
    struct unwrap_refwrapper {
        using type = T;
    };

    template <class T>
    struct unwrap_refwrapper<std::reference_wrapper<T>> {
        using type = T&;
    };

    template <class T>
    using unwrap_decay_t = typename unwrap_refwrapper<
        typename std::decay<T>::type>::type;
} // namespace futures::detail

#endif // FUTURES_DETAIL_TRAITS_UNWRAP_REFWRAPPER_HPP
