//
// execution.hpp
// ~~~~~~~~~~~~~
//
// Copyright (c) 2003-2020 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef BOOST_ASIO_EXECUTION_HPP
#define BOOST_ASIO_EXECUTION_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include <futures/detail/bundled/boost/asio/execution/allocator.hpp>
#include <futures/detail/bundled/boost/asio/execution/any_executor.hpp>
#include <futures/detail/bundled/boost/asio/execution/bad_executor.hpp>
#include <futures/detail/bundled/boost/asio/execution/blocking.hpp>
#include <futures/detail/bundled/boost/asio/execution/blocking_adaptation.hpp>
#include <futures/detail/bundled/boost/asio/execution/bulk_execute.hpp>
#include <futures/detail/bundled/boost/asio/execution/bulk_guarantee.hpp>
#include <futures/detail/bundled/boost/asio/execution/connect.hpp>
#include <futures/detail/bundled/boost/asio/execution/context.hpp>
#include <futures/detail/bundled/boost/asio/execution/context_as.hpp>
#include <futures/detail/bundled/boost/asio/execution/execute.hpp>
#include <futures/detail/bundled/boost/asio/execution/executor.hpp>
#include <futures/detail/bundled/boost/asio/execution/invocable_archetype.hpp>
#include <futures/detail/bundled/boost/asio/execution/mapping.hpp>
#include <futures/detail/bundled/boost/asio/execution/occupancy.hpp>
#include <futures/detail/bundled/boost/asio/execution/operation_state.hpp>
#include <futures/detail/bundled/boost/asio/execution/outstanding_work.hpp>
#include <futures/detail/bundled/boost/asio/execution/prefer_only.hpp>
#include <futures/detail/bundled/boost/asio/execution/receiver.hpp>
#include <futures/detail/bundled/boost/asio/execution/receiver_invocation_error.hpp>
#include <futures/detail/bundled/boost/asio/execution/relationship.hpp>
#include <futures/detail/bundled/boost/asio/execution/schedule.hpp>
#include <futures/detail/bundled/boost/asio/execution/scheduler.hpp>
#include <futures/detail/bundled/boost/asio/execution/sender.hpp>
#include <futures/detail/bundled/boost/asio/execution/set_done.hpp>
#include <futures/detail/bundled/boost/asio/execution/set_error.hpp>
#include <futures/detail/bundled/boost/asio/execution/set_value.hpp>
#include <futures/detail/bundled/boost/asio/execution/start.hpp>
#include <futures/detail/bundled/boost/asio/execution/submit.hpp>

#endif // BOOST_ASIO_EXECUTION_HPP
