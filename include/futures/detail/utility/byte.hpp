//
// Copyright (c) alandefreitas 2/18/22.
// See accompanying file LICENSE
//

#ifndef FUTURES_DETAIL_UTILITY_BYTE_HPP
#define FUTURES_DETAIL_UTILITY_BYTE_HPP

#include <type_traits>

namespace futures::detail {
    enum class byte : unsigned char
    {
    };

    template <
        class IntegerType,
        std::enable_if_t<std::is_integral_v<IntegerType>, int> = 0>
    constexpr IntegerType
    to_integer(byte b) noexcept {
        return IntegerType(b);
    }

    template <
        class IntegerType,
        std::enable_if_t<std::is_integral_v<IntegerType>, int> = 0>
    constexpr byte&
    operator<<=(byte& b, IntegerType shift) noexcept {
        return b = b << shift;
    }

    template <
        class IntegerType,
        std::enable_if_t<std::is_integral_v<IntegerType>, int> = 0>
    constexpr byte&
    operator>>=(byte& b, IntegerType shift) noexcept {
        return b = b >> shift;
    }

    template <
        class IntegerType,
        std::enable_if_t<std::is_integral_v<IntegerType>, int> = 0>
    constexpr byte
    operator<<(byte b, IntegerType shift) noexcept {
        return byte(static_cast<unsigned int>(b) << shift);
    }

    template <
        class IntegerType,
        std::enable_if_t<std::is_integral_v<IntegerType>, int> = 0>
    constexpr byte
    operator>>(byte b, IntegerType shift) noexcept {
        return byte(static_cast<unsigned int>(b) >> shift);
    }

    constexpr byte
    operator|(byte l, byte r) noexcept {
        return byte(
            static_cast<unsigned int>(l) | static_cast<unsigned int>(r));
    }

    constexpr byte
    operator&(byte l, byte r) noexcept {
        return byte(
            static_cast<unsigned int>(l) & static_cast<unsigned int>(r));
    }

    constexpr byte
    operator^(byte l, byte r) noexcept {
        return byte(
            static_cast<unsigned int>(l) ^ static_cast<unsigned int>(r));
    }

    constexpr byte
    operator~(byte b) noexcept {
        return byte(~static_cast<unsigned int>(b));
    }

    constexpr byte&
    operator|=(byte& l, byte r) noexcept {
        return l = l | r;
    }

    constexpr byte&
    operator&=(byte& l, byte r) noexcept {
        return l = l & r;
    }

    constexpr byte&
    operator^=(byte& l, byte r) noexcept {
        return l = l ^ r;
    }


} // namespace futures::detail

#endif // FUTURES_DETAIL_UTILITY_BYTE_HPP
