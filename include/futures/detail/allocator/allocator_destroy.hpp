//
// Copyright (c) 2022 alandefreitas (alandefreitas@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//

#ifndef FUTURES_DETAIL_ALLOCATOR_ALLOCATOR_DESTROY_HPP
#define FUTURES_DETAIL_ALLOCATOR_ALLOCATOR_DESTROY_HPP

#include <memory>

namespace futures::detail {
    /// \brief Convenience to destroy with allocator traits
    template <class A, class T>
    auto
    allocator_destroy(A& a, T* p) {
        return std::allocator_traits<A>::destroy(a, p);
    }

} // namespace futures::detail


#endif // FUTURES_DETAIL_ALLOCATOR_ALLOCATOR_DESTROY_HPP
