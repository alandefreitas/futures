//
// Copyright (c) 2022 alandefreitas (alandefreitas@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//

#ifndef FUTURES_DETAIL_DEPS_ASIO_EXECUTION_HPP
#define FUTURES_DETAIL_DEPS_ASIO_EXECUTION_HPP

#include <futures/config.hpp>

// Include asio/execution.hpp from external or bundled asio 
#if defined(FUTURES_USE_STANDALONE_ASIO)
#include <asio/execution.hpp>
#elif defined(FUTURES_USE_BOOST_ASIO)
#include <boost/asio/execution.hpp>
#else
#include <futures/detail/bundled/asio/execution.hpp>
#endif

#endif // FUTURES_DETAIL_DEPS_ASIO_EXECUTION_HPP