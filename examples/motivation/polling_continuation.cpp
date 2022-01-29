#include <futures/algorithm.hpp>
#include <futures/futures.hpp>
#include <iostream>

int main() {
    using namespace futures;

    // Waiting
    {
        std::future A = std::async([]() { return 2; });
        int A_result = A.get();
        std::cout << A_result << std::endl;
    }

    // Polling
    {
        std::future A = std::async([]() { return 2; });
        std::future B = std::async([&]() {
            int A_result = A.get();
            std::cout << A_result << std::endl;
        });
        B.wait();
    }

    // Lazy continuations
    {
        auto A = futures::async([]() { return 2; });
        auto B = futures::then(A, [](int A_result) {
            std::cout << A_result << std::endl;
        });
        B.wait();
    }

    return 0;
}