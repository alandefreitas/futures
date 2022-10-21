//
// Copyright (c) 2022 alandefreitas (alandefreitas@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//

#ifndef FUTURES_DETAIL_DEPS_ASIO_POST_HPP
#define FUTURES_DETAIL_DEPS_ASIO_POST_HPP

#include <futures/config.hpp>

// Include
#if defined(FUTURES_USE_STANDALONE_ASIO)
#include <asio/post.hpp>
#elif defined(FUTURES_USE_BOOST_ASIO)
#include <boost/asio/post.hpp>
#else
#include <futures/detail/bundled/asio/post.hpp>
#endif

#endif // FUTURES_DETAIL_DEPS_ASIO_POST_HPP