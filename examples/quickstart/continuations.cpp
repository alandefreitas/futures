#define FUTURES_DEFAULT_THREAD_POOL_SIZE 1
#include <futures/futures.hpp>
#include <iostream>

int
main() {
    using namespace futures;

    //[basic Basic
    auto f1 = async([]() -> int { return 42; });
    auto f1_cont = then(f1, [](int x) { return x * 2; });
    std::cout << f1_cont.get() << '\n';
    //]

    //[operator Operators
    auto f2 = std::async([]() -> int { return 63; });
    auto f2_cont = f2 >> [](int x) {
        return x * 2;
    };
    std::cout << f2_cont.get() << '\n';
    //]

    //[unwrapping Unwrapping parameters
    auto f3 = std::async([]() { return std::make_tuple(1,2.5,'c'); });
    auto f3_cont = f3 >> [](int x, double y, char z) {
        std::cout << x << ' ' << y << ' ' << z << '\n';
    };
    f3_cont.get();
    //]

    return 0;
}