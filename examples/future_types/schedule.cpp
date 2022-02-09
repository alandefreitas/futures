#include <futures/futures.hpp>
#include <iostream>

int
main() {
    using namespace futures;

    asio::thread_pool custom_pool(1);
    asio::thread_pool::executor_type ex = custom_pool.executor();

    //[schedule Scheduling deferred tasks
    auto f1 = schedule([] { std::cout << "No params\n"; });
    auto f2 = schedule([](int x) { std::cout << x << '\n'; }, 2);
    auto f3 = schedule([](int x, int y) { return x + y; }, 2, 3);
    auto f4 = schedule(ex, [] {
        std::cout << "custom executor\n";
    });
    auto f5 = schedule(make_inline_executor(), [] {
        std::cout << "custom executor\n";
    });

    // Tasks are only launched now!
    f1.wait();
    f2.wait();
    std::cout << f3.get() << '\n';
    f4.wait();
    f5.wait();
    //]
}