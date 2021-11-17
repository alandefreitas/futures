#include <futures/algorithm.h>
#include <futures/futures.h>
#include <iostream>

int main() {
    using namespace futures;

    auto future1 =
        futures::async([] { std::cout << "No params" << std::endl; });

    auto future2 =
        futures::async([](int x) { std::cout << x << std::endl; }, 2);

    auto future3 =
        futures::async([](int x, int y) { return x + y; }, 2, 3);

    asio::thread_pool custom_pool(1);
    asio::thread_pool::executor_type ex = custom_pool.executor();
    auto future4 = futures::async(
        ex, [] { std::cout << "custom executor" << std::endl; });

    auto future5 = futures::async(make_inline_executor(), [] {
        std::cout << "custom executor" << std::endl;
    });

    future1.wait();
    future2.wait();
    std::cout << future3.get() << std::endl;
    future4.wait();
    future5.wait();
}