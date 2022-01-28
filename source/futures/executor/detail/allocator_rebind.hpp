//
// Copyright (c) 2022 alandefreitas (alandefreitas@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//

#ifndef FUTURES_ALLOCATOR_REBIND_HPP
#define FUTURES_ALLOCATOR_REBIND_HPP

#include <memory>

namespace futures::detail {
    /// \brief Convenience traits to rebind allocators
    template <class A, class T>
    struct allocator_rebind
    {
        using type = typename std::allocator_traits<A>::template rebind_alloc<T>;
    };

    template <class A, class T>
    using allocator_rebind_t = typename allocator_rebind<A, T>::type;

} // namespace futures::detail


#endif // FUTURES_ALLOCATOR_REBIND_HPP
