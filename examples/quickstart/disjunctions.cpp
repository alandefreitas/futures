#include <futures/algorithm.hpp>
#include <futures/futures.hpp>
#include <iostream>

int main() {
    using namespace futures;

    auto f1 = futures::async([]() -> int { return 10; });
    auto f2 = futures::async([]() -> int { return 11; });
    auto f3 = futures::async([]() -> int { return 12; });
    auto f4 = futures::when_any(f1, f2, f3);
    auto f5 = futures::then(f4, [](int first_ready) {
        std::cout << first_ready << '\n';
    });
    f5.wait();

    auto f6 = futures::async([]() -> int { return 15; });
    auto f7 = futures::async([]() -> int { return 16; });
    auto f8 = f6 || f7;
    auto r = f8.get();
    if (r.index == 0) {
        std::cout << std::get<0>(r.tasks).get() << '\n';
    } else {
        std::cout << std::get<1>(r.tasks).get() << '\n';
    }

    return 0;
}