#include <futures/algorithm.hpp>
#include <futures/futures.hpp>
#include <iostream>

int
main() {
    using namespace futures;

    std::vector<int> v(50000);
    std::iota(v.begin(), v.end(), 1);

    // Parallel by default
    int c = futures::reduce(v, 0);
    std::cout << "Sum: " << c << '\n';

    // The launch policy can be replaced with a custom executor
    asio::thread_pool custom_pool2(4);
    asio::thread_pool::executor_type ex2 = custom_pool2.executor();
    futures::for_each(ex2, v.begin(), v.begin() + 10, [](int x) {
        std::cout << x << '\n';
    });

    // Use a custom partitioner
    auto p = [](std::vector<int>::iterator first,
                std::vector<int>::iterator last) {
        return std::next(first, (last - first) / 2);
    };
    auto it = find(ex2, p, v, 3000);
    if (it != v.end()) {
        std::cout << *it << " found at v[" << it - v.begin() << "]"
                  << '\n';
    }

    return 0;
}