//
// Copyright (c) 2022 alandefreitas (alandefreitas@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//

#ifndef FUTURES_ALGORITHM_TRAITS_IS_SENTINEL_FOR_HPP
#define FUTURES_ALGORITHM_TRAITS_IS_SENTINEL_FOR_HPP

#include <futures/algorithm/traits/is_input_or_output_iterator.hpp>
#include <futures/algorithm/traits/is_semiregular.hpp>
#include <type_traits>

namespace futures {
    /** @addtogroup algorithms Algorithms
     *  @{
     */

    /** @addtogroup traits Traits
     *  @{
     */


    /** \brief A C++17 type trait equivalent to the C++20 sentinel_for
     * concept
     */
#ifdef FUTURES_DOXYGEN
    template <class S, class I>
    using is_sentinel_for = __see_below__;
#else
    template <class S, class I, class = void>
    struct is_sentinel_for : std::false_type {};

    template <class S, class I>
    struct is_sentinel_for<
        S,
        I,
        std::enable_if_t<
            // clang-format off
            is_semiregular_v<S> &&
            is_input_or_output_iterator_v<I>
            // clang-format on
            >> : std::true_type {};
#endif
    template <class S, class I>
    constexpr bool is_sentinel_for_v = is_sentinel_for<S, I>::value;

    /** @}*/
    /** @}*/

} // namespace futures

#endif // FUTURES_ALGORITHM_TRAITS_IS_SENTINEL_FOR_HPP
