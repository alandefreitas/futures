//
// detail/completion_handler.hpp
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2021 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef ASIO_DETAIL_COMPLETION_HANDLER_HPP
#define ASIO_DETAIL_COMPLETION_HANDLER_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include <futures/detail/bundled/asio/detail/config.hpp>
#include <futures/detail/bundled/asio/detail/fenced_block.hpp>
#include <futures/detail/bundled/asio/detail/handler_alloc_helpers.hpp>
#include <futures/detail/bundled/asio/detail/handler_work.hpp>
#include <futures/detail/bundled/asio/detail/memory.hpp>
#include <futures/detail/bundled/asio/detail/operation.hpp>

#include <futures/detail/bundled/asio/detail/push_options.hpp>

namespace asio {
namespace detail {

template <typename Handler, typename IoExecutor>
class completion_handler : public operation
{
public:
  ASIO_DEFINE_HANDLER_PTR(completion_handler);

  completion_handler(Handler& h, const IoExecutor& io_ex)
    : operation(&completion_handler::do_complete),
      handler_(ASIO_MOVE_CAST(Handler)(h)),
      work_(handler_, io_ex)
  {
  }

  static void do_complete(void* owner, operation* base,
      const asio::error_code& /*ec*/,
      std::size_t /*bytes_transferred*/)
  {
    // Take ownership of the handler object.
    completion_handler* h(static_cast<completion_handler*>(base));
    ptr p = { asio::detail::addressof(h->handler_), h, h };

    ASIO_HANDLER_COMPLETION((*h));

    // Take ownership of the operation's outstanding work.
    handler_work<Handler, IoExecutor> w(
        ASIO_MOVE_CAST2(handler_work<Handler, IoExecutor>)(
          h->work_));

    // Make a copy of the handler so that the memory can be deallocated before
    // the upcall is made. Even if we're not about to make an upcall, a
    // sub-object of the handler may be the true owner of the memory associated
    // with the handler. Consequently, a local copy of the handler is required
    // to ensure that any owning sub-object remains valid until after we have
    // deallocated the memory here.
    Handler handler(ASIO_MOVE_CAST(Handler)(h->handler_));
    p.h = asio::detail::addressof(handler);
    p.reset();

    // Make the upcall if required.
    if (owner)
    {
      fenced_block b(fenced_block::half);
      ASIO_HANDLER_INVOCATION_BEGIN(());
      w.complete(handler, handler);
      ASIO_HANDLER_INVOCATION_END;
    }
  }

private:
  Handler handler_;
  handler_work<Handler, IoExecutor> work_;
};

} // namespace detail
} // namespace asio

#include <futures/detail/bundled/asio/detail/pop_options.hpp>

#endif // ASIO_DETAIL_COMPLETION_HANDLER_HPP
