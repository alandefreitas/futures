//
// Copyright (c) 2022 alandefreitas (alandefreitas@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//

#ifndef FUTURES_DETAIL_DEPS_BOOST_CONTAINER_SMALL_VECTOR_HPP
#define FUTURES_DETAIL_DEPS_BOOST_CONTAINER_SMALL_VECTOR_HPP

#include <futures/config.hpp>

#ifdef BOOST_GCC
#    pragma GCC diagnostic push
#    pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#endif

#if defined(BOOST_CLANG)
#    pragma clang diagnostic push
#    pragma clang diagnostic ignored "-Wdeprecated-declarations"
#endif

// Include boost/container/small_vector.hpp from external or bundled boost
#if defined(FUTURES_HAS_BOOST)
#    include <boost/container/small_vector.hpp>
#else
#    include <futures/detail/bundled/boost/container/small_vector.hpp>
#endif

#ifdef BOOST_CLANG
#    pragma clang diagnostic pop
#endif

#ifdef BOOST_GCC
#    pragma GCC diagnostic pop
#endif

#endif // FUTURES_DETAIL_DEPS_BOOST_CONTAINER_SMALL_VECTOR_HPP