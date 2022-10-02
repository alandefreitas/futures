//
// impl/src.hpp
// ~~~~~~~~~~~~
//
// Copyright (c) 2003-2020 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef BOOST_ASIO_IMPL_SRC_HPP
#define BOOST_ASIO_IMPL_SRC_HPP

#define BOOST_ASIO_SOURCE

#include <futures/detail/bundled/boost/asio/detail/config.hpp>

#if defined(BOOST_ASIO_HEADER_ONLY)
# error Do not compile Asio library source with BOOST_ASIO_HEADER_ONLY defined
#endif

#include <futures/detail/bundled/boost/asio/impl/error.ipp>
#include <futures/detail/bundled/boost/asio/impl/execution_context.ipp>
#include <futures/detail/bundled/boost/asio/impl/executor.ipp>
#include <futures/detail/bundled/boost/asio/impl/handler_alloc_hook.ipp>
#include <futures/detail/bundled/boost/asio/impl/io_context.ipp>
#include <futures/detail/bundled/boost/asio/impl/multiple_exceptions.ipp>
#include <futures/detail/bundled/boost/asio/impl/serial_port_base.ipp>
#include <futures/detail/bundled/boost/asio/impl/system_context.ipp>
#include <futures/detail/bundled/boost/asio/impl/thread_pool.ipp>
#include <futures/detail/bundled/boost/asio/detail/impl/buffer_sequence_adapter.ipp>
#include <futures/detail/bundled/boost/asio/detail/impl/descriptor_ops.ipp>
#include <futures/detail/bundled/boost/asio/detail/impl/dev_poll_reactor.ipp>
#include <futures/detail/bundled/boost/asio/detail/impl/epoll_reactor.ipp>
#include <futures/detail/bundled/boost/asio/detail/impl/eventfd_select_interrupter.ipp>
#include <futures/detail/bundled/boost/asio/detail/impl/handler_tracking.ipp>
#include <futures/detail/bundled/boost/asio/detail/impl/kqueue_reactor.ipp>
#include <futures/detail/bundled/boost/asio/detail/impl/null_event.ipp>
#include <futures/detail/bundled/boost/asio/detail/impl/pipe_select_interrupter.ipp>
#include <futures/detail/bundled/boost/asio/detail/impl/posix_event.ipp>
#include <futures/detail/bundled/boost/asio/detail/impl/posix_mutex.ipp>
#include <futures/detail/bundled/boost/asio/detail/impl/posix_thread.ipp>
#include <futures/detail/bundled/boost/asio/detail/impl/posix_tss_ptr.ipp>
#include <futures/detail/bundled/boost/asio/detail/impl/reactive_descriptor_service.ipp>
#include <futures/detail/bundled/boost/asio/detail/impl/reactive_serial_port_service.ipp>
#include <futures/detail/bundled/boost/asio/detail/impl/reactive_socket_service_base.ipp>
#include <futures/detail/bundled/boost/asio/detail/impl/resolver_service_base.ipp>
#include <futures/detail/bundled/boost/asio/detail/impl/scheduler.ipp>
#include <futures/detail/bundled/boost/asio/detail/impl/select_reactor.ipp>
#include <futures/detail/bundled/boost/asio/detail/impl/service_registry.ipp>
#include <futures/detail/bundled/boost/asio/detail/impl/signal_set_service.ipp>
#include <futures/detail/bundled/boost/asio/detail/impl/socket_ops.ipp>
#include <futures/detail/bundled/boost/asio/detail/impl/socket_select_interrupter.ipp>
#include <futures/detail/bundled/boost/asio/detail/impl/strand_executor_service.ipp>
#include <futures/detail/bundled/boost/asio/detail/impl/strand_service.ipp>
#include <futures/detail/bundled/boost/asio/detail/impl/throw_error.ipp>
#include <futures/detail/bundled/boost/asio/detail/impl/timer_queue_ptime.ipp>
#include <futures/detail/bundled/boost/asio/detail/impl/timer_queue_set.ipp>
#include <futures/detail/bundled/boost/asio/detail/impl/win_iocp_handle_service.ipp>
#include <futures/detail/bundled/boost/asio/detail/impl/win_iocp_io_context.ipp>
#include <futures/detail/bundled/boost/asio/detail/impl/win_iocp_serial_port_service.ipp>
#include <futures/detail/bundled/boost/asio/detail/impl/win_iocp_socket_service_base.ipp>
#include <futures/detail/bundled/boost/asio/detail/impl/win_event.ipp>
#include <futures/detail/bundled/boost/asio/detail/impl/win_mutex.ipp>
#include <futures/detail/bundled/boost/asio/detail/impl/win_object_handle_service.ipp>
#include <futures/detail/bundled/boost/asio/detail/impl/win_static_mutex.ipp>
#include <futures/detail/bundled/boost/asio/detail/impl/win_thread.ipp>
#include <futures/detail/bundled/boost/asio/detail/impl/win_tss_ptr.ipp>
#include <futures/detail/bundled/boost/asio/detail/impl/winrt_ssocket_service_base.ipp>
#include <futures/detail/bundled/boost/asio/detail/impl/winrt_timer_scheduler.ipp>
#include <futures/detail/bundled/boost/asio/detail/impl/winsock_init.ipp>
#include <futures/detail/bundled/boost/asio/execution/impl/bad_executor.ipp>
#include <futures/detail/bundled/boost/asio/execution/impl/receiver_invocation_error.ipp>
#include <futures/detail/bundled/boost/asio/generic/detail/impl/endpoint.ipp>
#include <futures/detail/bundled/boost/asio/ip/impl/address.ipp>
#include <futures/detail/bundled/boost/asio/ip/impl/address_v4.ipp>
#include <futures/detail/bundled/boost/asio/ip/impl/address_v6.ipp>
#include <futures/detail/bundled/boost/asio/ip/impl/host_name.ipp>
#include <futures/detail/bundled/boost/asio/ip/impl/network_v4.ipp>
#include <futures/detail/bundled/boost/asio/ip/impl/network_v6.ipp>
#include <futures/detail/bundled/boost/asio/ip/detail/impl/endpoint.ipp>
#include <futures/detail/bundled/boost/asio/local/detail/impl/endpoint.ipp>

#endif // BOOST_ASIO_IMPL_SRC_HPP
