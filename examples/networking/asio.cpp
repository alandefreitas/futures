//
// Copyright (c) 2022 alandefreitas (alandefreitas@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//

#include <futures/algorithm.hpp>
#include <futures/futures.hpp>
#include <iostream>

int
main() {
    using namespace futures;

    {
        //[enqueue asio::io_context as a task queue
        asio::io_context io;
        using io_executor = asio::io_context::executor_type;
        io_executor ex = io.get_executor();
        cfuture<int, io_executor> f = async(ex, []() { return 2; });
        //]

        //[pop asio::io_context as a task queue
        io.run();
        std::cout << f.get() << '\n'; // 2
        //]
    }

    {
        /*
        //[push_networking Push a networking task
        asio::io_context io;
        asio::ip::tcp::acceptor acceptor(io, asio::ip::tcp::endpoint(asio::ip::tcp::v4(), 13));
        asio::ip::tcp::socket client_socket(io);
        acceptor.async_accept(client_socket, [](const asio::error_code&) {
            // handle connection
        });
        //]

        //[pop_networking Pop task
        io.run();
        //]
        */
    }

    return 0;
}