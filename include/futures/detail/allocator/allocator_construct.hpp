//
// Copyright (c) 2022 alandefreitas (alandefreitas@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//

#ifndef FUTURES_DETAIL_ALLOCATOR_ALLOCATOR_CONSTRUCT_HPP
#define FUTURES_DETAIL_ALLOCATOR_ALLOCATOR_CONSTRUCT_HPP

#include <memory>

namespace futures::detail {
    /// Convenience to construct with allocator traits
    template <class A, class T, class... Args>
    auto
    allocator_construct(A& a, T* p, Args&&... args) {
        return std::allocator_traits<
            A>::construct(a, p, std::forward<Args>(args)...);
    }

} // namespace futures::detail


#endif // FUTURES_DETAIL_ALLOCATOR_ALLOCATOR_CONSTRUCT_HPP
