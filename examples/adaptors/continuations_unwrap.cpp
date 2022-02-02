#include <futures/algorithm.hpp>
#include <futures/futures.hpp>
#include <iostream>

int
main() {
    using namespace futures;

    cfuture<void> future1 = futures::async([]() {
        std::cout << "Nothing to unwrap" << '\n';
    });

    cfuture<int> future2 = future1 >> []() {
        return 6;
    };

    // forward int directly as int
    auto future3 = future2 >> [](int x) {
        return make_ready_future(x * 2);
    };

    // unwrap previous future<int> to int
    cfuture<std::tuple<int, int, int>> future4 = future3 >> [](int x) {
        return std::make_tuple(x * 1, x * 2, x * 3);
    };

    // unwrap tuple<int,int,int> to int, int, int
    cfuture<int> future5 = future4 >> [](int a, int b, int c) {
        return a * b * c;
    };

    // unwrap tuple of future<int>s to int,int,int
    cfuture<std::tuple<vfuture<int>, vfuture<int>, vfuture<int>>>
        future6 = future5 >> [](int x) {
            return std::make_tuple(
                make_ready_future(1 * x),
                make_ready_future(2 * x),
                make_ready_future(3 * x));
        };
    cfuture<int> future7 = future6 >> [](int a, int b, int c) {
        return a + b + c;
    };
    std::cout << future7.get() << '\n';

    return 0;
}