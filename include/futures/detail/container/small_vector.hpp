//
// Copyright (c) 2022 alandefreitas (alandefreitas@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//

#ifndef FUTURES_DETAIL_CONTAINER_SMALL_VECTOR_HPP
#define FUTURES_DETAIL_CONTAINER_SMALL_VECTOR_HPP

#include <futures/detail/deps/boost/container/small_vector.hpp>

namespace futures::detail {
    template <
        class T,
        size_t N
        = (std::max)(std::size_t(5), (sizeof(T *) + sizeof(size_t)) / sizeof(T)),
        class Allocator = std::allocator<T>>
    using small_vector = boost::container::small_vector<T, N, Allocator>;
} // namespace futures::detail

#endif // FUTURES_DETAIL_CONTAINER_SMALL_VECTOR_HPP
