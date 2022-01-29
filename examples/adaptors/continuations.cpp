#include <futures/algorithm.hpp>
#include <futures/futures.hpp>
#include <iostream>

int main() {
    using namespace futures;

    // Continuation to a continuable future
    cfuture<int> future1 = futures::async([]() -> int { return 42; });
    cfuture<void> future2 = futures::then(
        future1, [](int x) { std::cout << x * 2 << std::endl; });

    // Continuation to a std::future
    std::future<int> future3 = std::async([]() -> int { return 63; });
    cfuture<void> future4 = futures::then(
        future3, [](int x) { std::cout << x * 2 << std::endl; });

    // Continuation with another executor
    cfuture<int> future5 = async([] { return 2; });
    asio::thread_pool pool(1);
    asio::thread_pool::executor_type ex = pool.executor();
    cfuture<int> future6 =
        then(future5, ex, [](int v) { return v * 2; });

    // Continuation with operator>>
    auto future7 = future6 >> [](int x) { return x * 2; };
    std::cout << future7.get() << std::endl;
}