//
// Copyright (c) 2022 alandefreitas (alandefreitas@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//

#ifndef FUTURES_DETAIL_ALLOCATOR_MAYBE_EMPTY_ALLOCATOR_HPP
#define FUTURES_DETAIL_ALLOCATOR_MAYBE_EMPTY_ALLOCATOR_HPP

#include <futures/detail/utility/maybe_empty.hpp>

namespace futures::detail {
    FUTURES_MAYBE_EMPTY_TYPE(allocator)
    FUTURES_MAYBE_EMPTY_TYPE(node_allocator)
} // namespace futures::detail

#endif // FUTURES_DETAIL_ALLOCATOR_MAYBE_EMPTY_ALLOCATOR_HPP
