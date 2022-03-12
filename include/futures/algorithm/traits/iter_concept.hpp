//
// Copyright (c) 2022 alandefreitas (alandefreitas@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//

#ifndef FUTURES_ALGORITHM_TRAITS_ITER_CONCEPT_HPP
#define FUTURES_ALGORITHM_TRAITS_ITER_CONCEPT_HPP

#include <futures/algorithm/traits/has_iterator_traits_iterator_category.hpp>
#include <futures/algorithm/traits/has_iterator_traits_iterator_concept.hpp>
#include <futures/algorithm/traits/remove_cvref.hpp>
#include <iterator>
#include <type_traits>

namespace futures {
    /** @addtogroup algorithms Algorithms
     *  @{
     */

    /** @addtogroup traits Traits
     *  @{
     */


    /** \brief A C++17 type trait equivalent to the C++20 iter_concept
     * concept
     */
#ifdef FUTURES_DOXYGEN
    template <class T>
    using iter_concept = __see_below__;
#else
    template <class T, class = void>
    struct iter_concept
    {};

    template <class T>
    struct iter_concept<
        T,
        std::enable_if_t<
            // clang-format off
            has_iterator_traits_iterator_concept_v<remove_cvref_t<T>>
            // clang-format on
            >>
    {
        using type = typename std::iterator_traits<
            remove_cvref_t<T>>::iterator_concept;
    };

    template <class T>
    struct iter_concept<
        T,
        std::enable_if_t<
            // clang-format off
            !has_iterator_traits_iterator_concept_v<remove_cvref_t<T>> &&
            has_iterator_traits_iterator_category_v<remove_cvref_t<T>>
            // clang-format on
            >>
    {
        using type = typename std::iterator_traits<
            remove_cvref_t<T>>::iterator_category;
    };

    template <class T>
    struct iter_concept<
        T,
        std::enable_if_t<
            // clang-format off
            !has_iterator_traits_iterator_concept_v<remove_cvref_t<T>> &&
            !has_iterator_traits_iterator_category_v<remove_cvref_t<T>>
            // clang-format on
            >>
    {
        using type = std::random_access_iterator_tag;
    };
#endif
    template <class T>
    using iter_concept_t = typename iter_concept<T>::type;

    /** @}*/
    /** @}*/

} // namespace futures

#endif // FUTURES_ALGORITHM_TRAITS_ITER_CONCEPT_HPP
