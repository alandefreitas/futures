#include <futures/futures.hpp>
#include <iostream>

int
main() {
    using namespace futures;

    auto future1 = futures::schedule([] {
        std::cout << "No params" << '\n';
    });

    auto future2 = futures::
        schedule([](int x) { std::cout << x << '\n'; }, 2);

    auto future3 = futures::
        schedule([](int x, int y) { return x + y; }, 2, 3);

    asio::thread_pool custom_pool(1);
    asio::thread_pool::executor_type ex = custom_pool.executor();
    auto future4 = futures::schedule(ex, [] {
        std::cout << "custom executor" << '\n';
    });

    auto future5 = futures::schedule(make_inline_executor(), [] {
        std::cout << "custom executor" << '\n';
    });

    future1.wait();
    future2.wait();
    std::cout << future3.get() << '\n';
    future4.wait();
    future5.wait();
}