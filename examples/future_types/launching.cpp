#include <futures/algorithm.hpp>
#include <futures/futures.hpp>
#include <iostream>

int
main() {
    using namespace futures;

    auto future1 = futures::async([] {
        std::cout << "No params" << '\n';
    });

    auto future2 = futures::
        async([](int x) { std::cout << x << '\n'; }, 2);

    auto future3 = futures::
        async([](int x, int y) { return x + y; }, 2, 3);

    asio::thread_pool custom_pool(1);
    asio::thread_pool::executor_type ex = custom_pool.executor();
    auto future4 = futures::async(ex, [] {
        std::cout << "custom executor" << '\n';
    });

    auto future5 = futures::async(make_inline_executor(), [] {
        std::cout << "custom executor" << '\n';
    });

    future1.wait();
    future2.wait();
    std::cout << future3.get() << '\n';
    future4.wait();
    future5.wait();
}