#include <futures/futures.hpp>
#include <iostream>

int
main() {
    using namespace futures;

    cfuture<int> A = async([]() { return 1; });

    cfuture<char> B = A.then([](int v) {
        return static_cast<char>(v);
    });

    cfuture<void> C = then(B, [](char c) {
        std::cout << "Result " << c << '\n';
    });

    C.wait();

    return 0;
}