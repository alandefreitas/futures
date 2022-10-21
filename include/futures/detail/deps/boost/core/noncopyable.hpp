//
// Copyright (c) 2022 alandefreitas (alandefreitas@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//

#ifndef FUTURES_DETAIL_DEPS_BOOST_CORE_NONCOPYABLE_HPP
#define FUTURES_DETAIL_DEPS_BOOST_CORE_NONCOPYABLE_HPP

#include <futures/config.hpp>

// Include
#if defined(FUTURES_HAS_BOOST)
#include <boost/core/noncopyable.hpp>
#else
#include <futures/detail/bundled/boost/core/noncopyable.hpp>
#endif

#endif // FUTURES_DETAIL_DEPS_BOOST_CORE_NONCOPYABLE_HPP