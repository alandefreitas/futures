//
// Copyright (c) 2022 alandefreitas (alandefreitas@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//

#ifndef FUTURES_ALGORITHM_TRAITS_DETAIL_NESTED_ITERATOR_TRAITS_DIFFERENCE_TYPE_HPP
#define FUTURES_ALGORITHM_TRAITS_DETAIL_NESTED_ITERATOR_TRAITS_DIFFERENCE_TYPE_HPP

#include <iterator>

namespace futures::detail {
    template <class T>
    using nested_iterator_traits_difference_type_t = typename std::iterator_traits<
        T>::difference_type;
} // namespace futures::detail

#endif // FUTURES_ALGORITHM_TRAITS_DETAIL_NESTED_ITERATOR_TRAITS_DIFFERENCE_TYPE_HPP
