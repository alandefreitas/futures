#include <futures/algorithm.hpp>
#include <futures/futures.hpp>
#include <iostream>

int
main() {
    using namespace futures;

    //[conjunctions Conjunctions
    auto f1 = futures::async([] { std::cout << "f1" << '\n'; });
    auto f2 = futures::async([] { std::cout << "f2" << '\n'; });
    auto f3 = futures::async([] { std::cout << "f3" << '\n'; });
    auto f4 = futures::async([] { std::cout << "f4" << '\n'; });
    auto f5 = futures::when_all(f1, f2, f3, f4);
    f5.wait();
    //]

    //[operator Operators
    auto f6 = futures::async([] { return 6; });
    auto f7 = futures::async([] { return 7; });
    auto f8 = futures::async([] { return 8; });
    auto f9 = f6 && f7 && f8;
    //]

    //[unwrapping Unwrapping
    auto f10 = futures::then(f9, [](int a, int b, int c) {
        return a * b * c;
    });
    std::cout << f10.get() << '\n';
    //]

    return 0;
}