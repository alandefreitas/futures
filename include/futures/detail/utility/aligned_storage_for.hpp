//
// Copyright (c) alandefreitas 2/18/22.
// See accompanying file LICENSE
//

#ifndef FUTURES_ALIGNED_STORAGE_FOR_H
#define FUTURES_ALIGNED_STORAGE_FOR_H

#include <futures/detail/utility/byte.hpp>
#include <algorithm>

namespace futures::detail {
    template <class... Ts>
    struct aligned_storage_for
    {
    public:
        constexpr byte*
        data() {
            return data_;
        }

        [[nodiscard]] constexpr byte*
        data() const {
            return data_;
        }

        [[nodiscard]] constexpr std::size_t
        size() const {
            return (std::max)({ sizeof(Ts)... });
        }

    private:
        alignas(Ts...) byte data_[(std::max)({ sizeof(Ts)... })];
    };
} // namespace futures::detail

#endif // FUTURES_ALIGNED_STORAGE_FOR_H
