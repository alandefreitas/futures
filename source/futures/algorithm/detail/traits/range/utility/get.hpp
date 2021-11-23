/// \file
// Range v3 library
//
//  Copyright Eric Niebler 2013-present
//
//  Use, modification and distribution is subject to the
//  Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
//
// Project home: https://github.com/ericniebler/range-v3
//

#ifndef FUTURES_RANGES_UTILITY_GET_HPP
#define FUTURES_RANGES_UTILITY_GET_HPP

#include <futures/algorithm/detail/traits/range/meta/meta.hpp>

#include <futures/algorithm/detail/traits/range/concepts/concepts.hpp>

#include <futures/algorithm/detail/traits/range/detail/adl_get.hpp>

#include <futures/algorithm/detail/traits/range/detail/prologue.hpp>

namespace futures::detail {
    /// \addtogroup group-utility Utility
    /// @{
    ///

    /// \cond
    namespace _get_ {
        /// \endcond
        // clang-format off
        template<std::size_t I, typename TupleLike>
        constexpr auto CPP_auto_fun(get)(TupleLike &&t)
        (
            return ranges_detail::adl_get<I>(static_cast<TupleLike &&>(t))
        )
        template<typename T, typename TupleLike>
        constexpr auto CPP_auto_fun(get)(TupleLike &&t)
        (
            return ranges_detail::adl_get<T>(static_cast<TupleLike &&>(t))
        )
            // clang-format on

            template <typename T>
            T &get(futures::detail::meta::id_t<T> &value) noexcept {
            return value;
        }
        template <typename T> T const &get(futures::detail::meta::id_t<T> const &value) noexcept { return value; }
        template <typename T> T &&get(futures::detail::meta::id_t<T> &&value) noexcept { return static_cast<T &&>(value); }
        /// \cond
    } // namespace _get_
    using namespace _get_;
    /// \endcond

    /// @}
} // namespace futures::detail

#include <futures/algorithm/detail/traits/range/detail/epilogue.hpp>

#endif
