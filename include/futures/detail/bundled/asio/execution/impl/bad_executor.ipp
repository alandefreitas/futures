//
// exection/impl/bad_executor.ipp
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2021 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef ASIO_EXECUTION_IMPL_BAD_EXECUTOR_IPP
#define ASIO_EXECUTION_IMPL_BAD_EXECUTOR_IPP

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include <futures/detail/bundled/asio/detail/config.hpp>
#include <futures/detail/bundled/asio/execution/bad_executor.hpp>

#include <futures/detail/bundled/asio/detail/push_options.hpp>

namespace asio {
namespace execution {

bad_executor::bad_executor() ASIO_NOEXCEPT
{
}

const char* bad_executor::what() const ASIO_NOEXCEPT_OR_NOTHROW
{
  return "bad executor";
}

} // namespace execution
} // namespace asio

#include <futures/detail/bundled/asio/detail/pop_options.hpp>

#endif // ASIO_EXECUTION_IMPL_BAD_EXECUTOR_IPP
