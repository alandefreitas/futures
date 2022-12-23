//
// Copyright (c) 2022 alandefreitas (alandefreitas@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//

#ifndef FUTURES_ALGORITHM_TRAITS_DETAIL_NESTED_ELEMENT_TYPE_HPP
#define FUTURES_ALGORITHM_TRAITS_DETAIL_NESTED_ELEMENT_TYPE_HPP

namespace futures {
    namespace detail {
        template <class T>
        using nested_element_type_t = typename T::element_type;
    } // namespace detail
} // namespace futures

#endif // FUTURES_ALGORITHM_TRAITS_DETAIL_NESTED_ELEMENT_TYPE_HPP
