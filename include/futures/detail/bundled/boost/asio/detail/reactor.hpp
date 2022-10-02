//
// detail/reactor.hpp
// ~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2020 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef BOOST_ASIO_DETAIL_REACTOR_HPP
#define BOOST_ASIO_DETAIL_REACTOR_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include <futures/detail/bundled/boost/asio/detail/reactor_fwd.hpp>

#if defined(BOOST_ASIO_HAS_EPOLL)
#include <futures/detail/bundled/boost/asio/detail/epoll_reactor.hpp>
#elif defined(BOOST_ASIO_HAS_KQUEUE)
#include <futures/detail/bundled/boost/asio/detail/kqueue_reactor.hpp>
#elif defined(BOOST_ASIO_HAS_DEV_POLL)
#include <futures/detail/bundled/boost/asio/detail/dev_poll_reactor.hpp>
#elif defined(BOOST_ASIO_HAS_IOCP) || defined(BOOST_ASIO_WINDOWS_RUNTIME)
#include <futures/detail/bundled/boost/asio/detail/null_reactor.hpp>
#else
#include <futures/detail/bundled/boost/asio/detail/select_reactor.hpp>
#endif

#endif // BOOST_ASIO_DETAIL_REACTOR_HPP
