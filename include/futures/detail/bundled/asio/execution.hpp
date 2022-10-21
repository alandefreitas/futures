//
// execution.hpp
// ~~~~~~~~~~~~~
//
// Copyright (c) 2003-2021 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef ASIO_EXECUTION_HPP
#define ASIO_EXECUTION_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include <futures/detail/bundled/asio/execution/allocator.hpp>
#include <futures/detail/bundled/asio/execution/any_executor.hpp>
#include <futures/detail/bundled/asio/execution/bad_executor.hpp>
#include <futures/detail/bundled/asio/execution/blocking.hpp>
#include <futures/detail/bundled/asio/execution/blocking_adaptation.hpp>
#include <futures/detail/bundled/asio/execution/bulk_execute.hpp>
#include <futures/detail/bundled/asio/execution/bulk_guarantee.hpp>
#include <futures/detail/bundled/asio/execution/connect.hpp>
#include <futures/detail/bundled/asio/execution/context.hpp>
#include <futures/detail/bundled/asio/execution/context_as.hpp>
#include <futures/detail/bundled/asio/execution/execute.hpp>
#include <futures/detail/bundled/asio/execution/executor.hpp>
#include <futures/detail/bundled/asio/execution/invocable_archetype.hpp>
#include <futures/detail/bundled/asio/execution/mapping.hpp>
#include <futures/detail/bundled/asio/execution/occupancy.hpp>
#include <futures/detail/bundled/asio/execution/operation_state.hpp>
#include <futures/detail/bundled/asio/execution/outstanding_work.hpp>
#include <futures/detail/bundled/asio/execution/prefer_only.hpp>
#include <futures/detail/bundled/asio/execution/receiver.hpp>
#include <futures/detail/bundled/asio/execution/receiver_invocation_error.hpp>
#include <futures/detail/bundled/asio/execution/relationship.hpp>
#include <futures/detail/bundled/asio/execution/schedule.hpp>
#include <futures/detail/bundled/asio/execution/scheduler.hpp>
#include <futures/detail/bundled/asio/execution/sender.hpp>
#include <futures/detail/bundled/asio/execution/set_done.hpp>
#include <futures/detail/bundled/asio/execution/set_error.hpp>
#include <futures/detail/bundled/asio/execution/set_value.hpp>
#include <futures/detail/bundled/asio/execution/start.hpp>
#include <futures/detail/bundled/asio/execution/submit.hpp>

#endif // ASIO_EXECUTION_HPP
