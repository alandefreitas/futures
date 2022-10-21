//
// detail/reactor.hpp
// ~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2021 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef ASIO_DETAIL_REACTOR_HPP
#define ASIO_DETAIL_REACTOR_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include <futures/detail/bundled/asio/detail/config.hpp>

#if defined(ASIO_HAS_IOCP) || defined(ASIO_WINDOWS_RUNTIME)
#include <futures/detail/bundled/asio/detail/null_reactor.hpp>
#elif defined(ASIO_HAS_IO_URING_AS_DEFAULT)
#include <futures/detail/bundled/asio/detail/null_reactor.hpp>
#elif defined(ASIO_HAS_EPOLL)
#include <futures/detail/bundled/asio/detail/epoll_reactor.hpp>
#elif defined(ASIO_HAS_KQUEUE)
#include <futures/detail/bundled/asio/detail/kqueue_reactor.hpp>
#elif defined(ASIO_HAS_DEV_POLL)
#include <futures/detail/bundled/asio/detail/dev_poll_reactor.hpp>
#else
#include <futures/detail/bundled/asio/detail/select_reactor.hpp>
#endif

namespace asio {
namespace detail {

#if defined(ASIO_HAS_IOCP) || defined(ASIO_WINDOWS_RUNTIME)
typedef null_reactor reactor;
#elif defined(ASIO_HAS_IO_URING_AS_DEFAULT)
typedef null_reactor reactor;
#elif defined(ASIO_HAS_EPOLL)
typedef epoll_reactor reactor;
#elif defined(ASIO_HAS_KQUEUE)
typedef kqueue_reactor reactor;
#elif defined(ASIO_HAS_DEV_POLL)
typedef dev_poll_reactor reactor;
#else
typedef select_reactor reactor;
#endif

} // namespace detail
} // namespace asio

#endif // ASIO_DETAIL_REACTOR_HPP
