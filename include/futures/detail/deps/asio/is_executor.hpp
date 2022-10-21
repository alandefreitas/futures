//
// Copyright (c) 2022 alandefreitas (alandefreitas@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//

#ifndef FUTURES_DETAIL_DEPS_ASIO_IS_EXECUTOR_HPP
#define FUTURES_DETAIL_DEPS_ASIO_IS_EXECUTOR_HPP

#include <futures/config.hpp>

// Include asio/is_executor.hpp from external or bundled asio 
#if defined(FUTURES_USE_STANDALONE_ASIO)
#include <asio/is_executor.hpp>
#elif defined(FUTURES_USE_BOOST_ASIO)
#include <boost/asio/is_executor.hpp>
#else
#include <futures/detail/bundled/asio/is_executor.hpp>
#endif

#endif // FUTURES_DETAIL_DEPS_ASIO_IS_EXECUTOR_HPP