//////////////////////////////////////////////////////////////////////////////
//
// (C) Copyright Ion Gaztanaga 2014-2014.
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/container for documentation.
//
//////////////////////////////////////////////////////////////////////////////

#ifndef BOOST_CONTAINER_DETAIL_CONSTRUCT_IN_PLACE_HPP
#define BOOST_CONTAINER_DETAIL_CONSTRUCT_IN_PLACE_HPP

#ifndef BOOST_CONFIG_HPP
#include <futures/detail/bundled/boost/config.hpp>
#endif

#if defined(BOOST_HAS_PRAGMA_ONCE)
#  pragma once
#endif

#include <futures/detail/bundled/boost/container/allocator_traits.hpp>
#include <futures/detail/bundled/boost/container/detail/iterators.hpp>
#include <futures/detail/bundled/boost/container/detail/value_init.hpp>

namespace boost {
namespace container {

//In place construction

struct iterator_arg_t{};

template<class Allocator, class T, class InpIt>
BOOST_CONTAINER_FORCEINLINE void construct_in_place(Allocator &a, T* dest, InpIt source)
{     boost::container::allocator_traits<Allocator>::construct(a, dest, *source);  }

template<class Allocator, class T, class U>
BOOST_CONTAINER_FORCEINLINE void construct_in_place(Allocator &a, T *dest, value_init_construct_iterator<U>)
{
   boost::container::allocator_traits<Allocator>::construct(a, dest);
}

template <class T>
class default_init_construct_iterator;

template<class Allocator, class T, class U>
BOOST_CONTAINER_FORCEINLINE void construct_in_place(Allocator &a, T *dest, default_init_construct_iterator<U>)
{
   boost::container::allocator_traits<Allocator>::construct(a, dest, default_init);
}

template <class T, class EmplaceFunctor>
class emplace_iterator;

template<class Allocator, class T, class U, class EF>
BOOST_CONTAINER_FORCEINLINE void construct_in_place(Allocator &a, T *dest, emplace_iterator<U, EF> ei)
{
   ei.construct_in_place(a, dest);
}

//Assignment

template<class DstIt, class InpIt>
BOOST_CONTAINER_FORCEINLINE void assign_in_place(DstIt dest, InpIt source)
{  *dest = *source;  }

template<class DstIt, class U>
BOOST_CONTAINER_FORCEINLINE void assign_in_place(DstIt dest, value_init_construct_iterator<U>)
{
   dtl::value_init<U> val;
   *dest = boost::move(val.get());
}

template <class DstIt>
class default_init_construct_iterator;

template<class DstIt, class U, class D>
BOOST_CONTAINER_FORCEINLINE void assign_in_place(DstIt dest, default_init_construct_iterator<U>)
{
   U u;
   *dest = boost::move(u);
}

template <class T, class EmplaceFunctor>
class emplace_iterator;

template<class DstIt, class U, class EF>
BOOST_CONTAINER_FORCEINLINE void assign_in_place(DstIt dest, emplace_iterator<U, EF> ei)
{
   ei.assign_in_place(dest);
}

}  //namespace container {
}  //namespace boost {

#endif   //#ifndef BOOST_CONTAINER_DETAIL_CONSTRUCT_IN_PLACE_HPP
