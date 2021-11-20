//
// Created by alandefreitas on 10/25/21.
//
// This example is an adapted version of the echo server from the ASIO
// docs.
//

#include <cstdlib>
#include <iostream>
#include <memory>
#include <string_view>
#include <utility>

#include <futures/futures.h>

#ifdef FUTURES_USE_ASIO
#include <asio.hpp>
#include <asio/ts/buffer.hpp>
#include <asio/ts/internet.hpp>
#else
#include <boost/asio.hpp>
#include <boost/asio/ts/buffer.hpp>
#include <boost/asio/ts/internet.hpp>
#endif

using futures::asio::ip::tcp;

class session : public std::enable_shared_from_this<session> {
  public:
    static constexpr size_t max_length = 1024;

  public:
    explicit session(tcp::socket socket)
        : socket_(std::move(socket)) {}

    void start() { schedule_read(); }

  private:
    void schedule_read() {
        socket_.async_read_some(
            futures::asio::buffer(data_, max_length),
            [this, self = shared_from_this()](std::error_code ec,
                                              std::size_t length) {
                if (!ec) {
                    std::cout
                        << std::string_view(
                               data_, std::min(max_length, length))
                        << std::endl;
                    schedule_write(length);
                }
            });
    }

    void schedule_write(std::size_t length) {
        // keep creating shared_from_this() pointer everytime we
        // schedule some function so that "this" object never dies
        futures::asio::async_write(
            socket_, futures::asio::buffer(data_, length),
            [this, self = shared_from_this()](
                std::error_code ec, std::size_t /*length*/) {
                if (!ec) {
                    schedule_read();
                }
            });
    }

    tcp::socket socket_;
    char data_[max_length]{};
};

class server {
  public:
    server(futures::asio::io_context &io_context, short port)
        : acceptor_(io_context, tcp::endpoint(tcp::v4(), port)),
          socket_(io_context), ex_(io_context.get_executor()) {
        schedule_accept();
    }

  private:
    void schedule_accept() {
        // Schedule function to accept connection from client
        std::future<void> client_connected =
            acceptor_.async_accept(socket_, futures::asio::use_future);

        // Attach continuation so that when the client connects, we
        // create a new session
        futures::cfuture<void> session_created =
            futures::then(ex_, client_connected, [this]() {
                std::cout << "Server log: New client" << std::endl;
                // Session is created with a shared pointer. The
                // internals of session ensure this pointer is kept
                // alive.
                std::make_shared<session>(std::move(socket_))
                    ->start();
                // Create one more task accepting connections
                schedule_accept();
            });

        // Schedule the tasks but don't wait for them to finish. It's
        // all detached. client_connected.detach();
        session_created.detach();
    }

    tcp::acceptor acceptor_;
    tcp::socket socket_;
    futures::asio::io_context::executor_type ex_;
};

int main(int argc, char *argv[]) {
    try {
        if (argc != 2) {
            std::cerr << "Usage: async_tcp_echo_server <port>\n";
            return 1;
        }

        futures::asio::io_context io_context;

        server s(io_context, std::atoi(argv[1]));

        std::thread t([&] { io_context.run(); });
        io_context.run();
        t.join();
    } catch (std::exception &e) {
        std::cerr << "Exception: " << e.what() << "\n";
    }

    return 0;
}