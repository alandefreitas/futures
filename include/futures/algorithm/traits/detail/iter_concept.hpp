//
// Copyright (c) 2022 alandefreitas (alandefreitas@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//

#ifndef FUTURES_ALGORITHM_TRAITS_DETAIL_ITER_CONCEPT_HPP
#define FUTURES_ALGORITHM_TRAITS_DETAIL_ITER_CONCEPT_HPP

#include <futures/algorithm/traits/remove_cvref.hpp>
#include <futures/algorithm/traits/detail/has_iterator_traits_iterator_category.hpp>
#include <futures/algorithm/traits/detail/has_iterator_traits_iterator_concept.hpp>
#include <iterator>
#include <type_traits>

namespace futures {
    namespace detail {
        template <class T>
        using iter_concept = mp_cond<
            detail::has_iterator_traits_iterator_concept<remove_cvref_t<T>>,
            mp_defer<
                nested_iterator_traits_iterator_concept_t,
                remove_cvref_t<T>>,
            detail::has_iterator_traits_iterator_category<remove_cvref_t<T>>,
            mp_defer<
                nested_iterator_traits_iterator_category_t,
                remove_cvref_t<T>>,
            std::true_type,
            mp_identity<std::random_access_iterator_tag>>;

        template <class T>
        using iter_concept_t = typename iter_concept<T>::type;
    } // namespace detail
} // namespace futures

#endif // FUTURES_ALGORITHM_TRAITS_DETAIL_ITER_CONCEPT_HPP
