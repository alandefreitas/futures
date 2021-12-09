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

#ifndef FUTURES_RANGES_UTILITY_IN_PLACE_HPP
#define FUTURES_RANGES_UTILITY_IN_PLACE_HPP

#include <futures/algorithm/detail/traits/range/range_fwd.h>

#include <futures/algorithm/detail/traits/range/utility/static_const.h>

#include <futures/algorithm/detail/traits/range/detail/prologue.h>

namespace futures::detail {
    /// \ingroup group-utility
    struct in_place_t {};
    RANGES_INLINE_VARIABLE(in_place_t, in_place)
} // namespace futures::detail

#include <futures/algorithm/detail/traits/range/detail/epilogue.h>

#endif
