#include <futures/algorithm.hpp>
#include <futures/futures.hpp>
#include <iostream>

int
main() {
    using namespace futures;

    //[continuable Continuation to a continuable future
    cfuture<int> f1 = async([]() -> int { return 42; });
    cfuture<void> f2 = then(f1, [](int x) {
        std::cout << x * 2 << '\n';
    });
    //]

    //[std_future Continuation to a std::future
    std::future<int> f3 = std::async([]() -> int { return 63; });
    cfuture<void> f4 = then(f3, [](int x) {
        std::cout << x * 2 << '\n';
    });
    //]

    //[deferred Continuation to a deferred future
    auto f5 = schedule([]() -> int { return 63; });
    auto f6 = then(f5, [](int x) { std::cout << x * 2 << '\n'; });
    //]

    //[executor Continuation with another executor
    cfuture<int> f7 = async([] { return 2; });
    asio::thread_pool pool(1);
    auto ex = pool.executor();
    cfuture<int> f8 = then(ex, f7, [](int v) { return v * 2; });
    //]

    //[operator operator>>
    cfuture<int> f9 = f8 >> [](int x) {
        return x * 2;
    };
    //]

    //[operator_ex operators >> and %
    auto inline_executor = make_inline_executor();
    auto f10 = f9 >> inline_executor % [](int x) {
        return x + 2;
    };
    //]

    std::cout << f10.get() << '\n';
}