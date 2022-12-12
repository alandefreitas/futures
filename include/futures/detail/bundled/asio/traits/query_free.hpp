//
// traits/query_free.hpp
// ~~~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2021 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef ASIO_TRAITS_QUERY_FREE_HPP
#define ASIO_TRAITS_QUERY_FREE_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include <futures/detail/bundled/asio/detail/config.hpp>
#include <futures/detail/bundled/asio/detail/type_traits.hpp>

#if defined(ASIO_HAS_DECLTYPE) \
  && defined(ASIO_HAS_NOEXCEPT) \
  && defined(ASIO_HAS_WORKING_EXPRESSION_SFINAE)
# define ASIO_HAS_DEDUCED_QUERY_FREE_TRAIT 1
#endif // defined(ASIO_HAS_DECLTYPE)
       //   && defined(ASIO_HAS_NOEXCEPT)
       //   && defined(ASIO_HAS_WORKING_EXPRESSION_SFINAE)

#include <futures/detail/bundled/asio/detail/push_options.hpp>

namespace asio {
namespace traits {

template <typename T, typename Property, typename = void>
struct query_free_default;

template <typename T, typename Property, typename = void>
struct query_free;

} // namespace traits
namespace detail {

struct no_query_free
{
  ASIO_STATIC_CONSTEXPR(bool, is_valid = false);
  ASIO_STATIC_CONSTEXPR(bool, is_noexcept = false);
};

#if defined(ASIO_HAS_DEDUCED_QUERY_FREE_TRAIT)

template <typename T, typename Property, typename = void>
struct query_free_trait : no_query_free
{
};

template <typename T, typename Property>
struct query_free_trait<T, Property,
  typename void_type<
    decltype(query(declval<T>(), declval<Property>()))
  >::type>
{
  ASIO_STATIC_CONSTEXPR(bool, is_valid = true);

  using result_type = decltype(
    query(declval<T>(), declval<Property>()));

  ASIO_STATIC_CONSTEXPR(bool, is_noexcept = noexcept(
    query(declval<T>(), declval<Property>())));
};

#else // defined(ASIO_HAS_DEDUCED_QUERY_FREE_TRAIT)

template <typename T, typename Property, typename = void>
struct query_free_trait :
  conditional<
    is_same<T, typename decay<T>::type>::value
      && is_same<Property, typename decay<Property>::type>::value,
    no_query_free,
    traits::query_free<
      typename decay<T>::type,
      typename decay<Property>::type>
  >::type
{
};

#endif // defined(ASIO_HAS_DEDUCED_QUERY_FREE_TRAIT)

} // namespace detail
namespace traits {

template <typename T, typename Property, typename>
struct query_free_default :
  detail::query_free_trait<T, Property>
{
};

template <typename T, typename Property, typename>
struct query_free :
  query_free_default<T, Property>
{
};

} // namespace traits
} // namespace asio

#include <futures/detail/bundled/asio/detail/pop_options.hpp>

#endif // ASIO_TRAITS_QUERY_FREE_HPP