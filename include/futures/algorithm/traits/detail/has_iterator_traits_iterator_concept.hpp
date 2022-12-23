//
// Copyright (c) 2022 alandefreitas (alandefreitas@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//

#ifndef FUTURES_ALGORITHM_TRAITS_DETAIL_HAS_ITERATOR_TRAITS_ITERATOR_CONCEPT_HPP
#define FUTURES_ALGORITHM_TRAITS_DETAIL_HAS_ITERATOR_TRAITS_ITERATOR_CONCEPT_HPP

#include <futures/config.hpp>
#include <futures/detail/deps/boost/mp11/utility.hpp>
#include <iterator>
#include <type_traits>

namespace futures {
    namespace detail {
        template <class T>
        using nested_iterator_traits_iterator_concept_t = typename std::
            iterator_traits<T>::iterator_concept;

        template <class T>
        using has_iterator_traits_iterator_concept
            = mp_valid<nested_iterator_traits_iterator_concept_t, T>;

        template <class T>
        constexpr bool has_iterator_traits_iterator_concept_v
            = has_iterator_traits_iterator_concept<T>::value;
    } // namespace detail
} // namespace futures

#endif // FUTURES_ALGORITHM_TRAITS_DETAIL_HAS_ITERATOR_TRAITS_ITERATOR_CONCEPT_HPP
