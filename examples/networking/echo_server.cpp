//
// Created by alandefreitas on 10/25/21.
//
// This example is an adapted version of the echo server from the ASIO
// docs. This is an example we are still going to adapt to use futures
// and asio executors.
//

#include <futures/futures.hpp>
#include <charconv>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <utility>
#include <string_view>

#ifdef FUTURES_USE_ASIO

#    include <asio/ts/buffer.hpp>
#    include <asio/ts/internet.hpp>
#    include <asio.hpp>

#else
#    include <boost/asio.hpp>
#    include <boost/asio/ts/buffer.hpp>
#    include <boost/asio/ts/internet.hpp>
#endif

using futures::asio::ip::tcp;

/// \brief Coroutine class representing a user session
///
/// This is a manual coroutine class representing a user session. For
/// this echo server, the session only includes the data we just read,
/// so we can write it back to the client.
///
struct session_coroutine
    : public std::enable_shared_from_this<session_coroutine>
{
public:
    static constexpr size_t max_length = 1024;

    explicit session_coroutine(tcp::socket socket)
        : socket_(std::move(socket)) {}

    void
    resume() {
        switch (state) {
        case Reading:
            socket_.async_read_some(
                futures::asio::buffer(data_, max_length),
                [this, self = shared_from_this()](
                    std::error_code ec,
                    std::size_t length) {
                if (!ec) {
                    std::cout << std::string_view(
                        data_.data(),
                        std::min(max_length, length))
                              << '\n';
                    state = Writing;
                    read_length = length;
                    resume();
                } else {
                    state = Done;
                }
                });
            return;
        case Writing:
            futures::asio::async_write(
                socket_,
                futures::asio::buffer(data_, read_length),
                [this, self = shared_from_this()](
                    std::error_code ec,
                    std::size_t /*length*/) {
                if (!ec) {
                    state = Reading;
                    resume();
                } else {
                    state = Done;
                }
                });
            return;
        default:
            return;
        }
    }

private:
    tcp::socket socket_;
    std::array<char, max_length> data_{};
    std::size_t read_length{ 0 };
    enum
    {
        Reading,
        Writing,
        Done
    } state{ Reading };
};

/// \brief Echo server
///
/// This class stores the all active session we have with clients and
/// schedules the acceptor to create new sessions.
///
class server
{
public:
    server(futures::asio::io_context &io_context, short port)
        : acceptor_(io_context, tcp::endpoint(tcp::v4(), port)),
          socket_(io_context), ex_(io_context.get_executor()) {
        schedule_accept();
    }

private:
    void
    schedule_accept() {
        // Schedule function to accept connection from client
        std::future<void> client_connected = acceptor_.async_accept(
            socket_,
            futures::asio::use_future);

        // Attach continuation so that when the client connects, we
        // create a new session
        auto session_created = futures::
            then(ex_, client_connected, [this]() {
                std::cout << "Server log: New client" << '\n';
                // Session is created with a shared pointer. The
                // internals of session ensure this pointer is kept
                // alive.
                std::make_shared<session_coroutine>(
                    std::move(socket_))
                    ->resume();
                // Create one more task accepting connections
                schedule_accept();
            });

        // Schedule the tasks but don't wait for them to finish. It's
        // all detached. `client_connected.detach();`
        session_created.detach();
    }

    tcp::acceptor acceptor_;
    tcp::socket socket_;
    futures::asio::io_context::executor_type ex_;
};

int
main(int argc, char *argv[]) {
    try {
        // Parse cli parameters
        short port = 8080;
        if (argc < 2) {
            std::cerr << "Usage: async_tcp_echo_server <port>\n";
        } else {
            std::string_view port_str = argv[1];
            auto [ptr, ec] = std::from_chars(
                port_str.data(),
                port_str.data() + port_str.size(),
                port,
                10);
            if (ec != std::errc() || ptr != port_str.data()) {
                std::cerr
                    << "Invalid port number " << port_str << '\n';
            }
        }
        std::cout << "http://localhost:" << port << '\n';

        // Create server
        futures::asio::io_context io_context;
        server s(io_context, port);

        // Launch threads running io tasks
        futures::asio::thread_pool pool(
            futures::hardware_concurrency());
        for (size_t i = 0; i < futures::hardware_concurrency(); ++i) {
            futures::asio::post(pool, [&]() { io_context.run(); });
        }
        io_context.run();
        pool.join();
    }
    catch (std::exception const &e) {
        std::cerr << "Exception: " << e.what() << "\n";
    }

    return 0;
}