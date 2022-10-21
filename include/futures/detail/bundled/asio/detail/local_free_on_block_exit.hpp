//
// detail/local_free_on_block_exit.hpp
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2021 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef ASIO_DETAIL_LOCAL_FREE_ON_BLOCK_EXIT_HPP
#define ASIO_DETAIL_LOCAL_FREE_ON_BLOCK_EXIT_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include <futures/detail/bundled/asio/detail/config.hpp>

#if defined(ASIO_WINDOWS) || defined(__CYGWIN__)
#if !defined(ASIO_WINDOWS_APP)

#include <futures/detail/bundled/asio/detail/noncopyable.hpp>
#include <futures/detail/bundled/asio/detail/socket_types.hpp>

#include <futures/detail/bundled/asio/detail/push_options.hpp>

namespace asio {
namespace detail {

class local_free_on_block_exit
  : private noncopyable
{
public:
  // Constructor blocks all signals for the calling thread.
  explicit local_free_on_block_exit(void* p)
    : p_(p)
  {
  }

  // Destructor restores the previous signal mask.
  ~local_free_on_block_exit()
  {
    ::LocalFree(p_);
  }

private:
  void* p_;
};

} // namespace detail
} // namespace asio

#include <futures/detail/bundled/asio/detail/pop_options.hpp>

#endif // !defined(ASIO_WINDOWS_APP)
#endif // defined(ASIO_WINDOWS) || defined(__CYGWIN__)

#endif // ASIO_DETAIL_LOCAL_FREE_ON_BLOCK_EXIT_HPP
