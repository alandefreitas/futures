#define FUTURES_DEFAULT_THREAD_POOL_SIZE 1
#include <futures/futures.hpp>
#include <iostream>

int
main() {
    using namespace futures;

    auto f1 = futures::async([]() -> int { return 42; });
    auto f1_cont = futures::then(f1, [](int x) { return x * 2; });

    auto f2 = std::async([]() -> int { return 63; });
    auto f2_cont = f2 >> [](int x) {
        return x * 2;
    };

    std::cout << f1_cont.get() << '\n';
    std::cout << f2_cont.get() << '\n';

    return 0;
}