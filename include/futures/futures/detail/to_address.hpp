//
// Copyright (c) 2021 alandefreitas (alandefreitas@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//

#ifndef FUTURES_TO_ADDRESS_H
#define FUTURES_TO_ADDRESS_H

/// \file
/// Replicate the C++20 to_address functionality in C++17

#include <memory>

namespace futures::detail {
    /// \brief Obtain the address represented by p without forming a reference
    /// to the object pointed to by p This is the "fancy pointer" overload: If
    /// the expression std::pointer_traits<Ptr>::to_address(p) is well-formed,
    /// returns the result of that expression. Otherwise, returns
    /// std::to_address(p.operator->()). \tparam T Element type \param v Element
    /// pointer \return Element address
    template <class T>
    constexpr T *
    to_address(T *v) noexcept {
        return v;
    }

    /// \brief Obtain the address represented by p without forming a reference
    /// to the object pointed to by p This is the "raw pointer overload": If T
    /// is a function type, the program is ill-formed. Otherwise, returns p
    /// unmodified. \tparam T Element type \param v Raw pointer \return Element
    /// address
    template <class T>
    inline typename std::pointer_traits<T>::element_type *
    to_address(const T &v) noexcept {
        return to_address(v.operator->());
    }
} // namespace futures::detail

#endif // FUTURES_TO_ADDRESS_H
