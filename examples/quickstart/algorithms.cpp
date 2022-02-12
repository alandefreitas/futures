#include <futures/algorithm.hpp>
#include <futures/futures.hpp>
#include <iostream>

int
main() {
    using namespace futures;

    //[algorithms Algorithms
    std::vector<int> v(50000);
    std::iota(v.begin(), v.end(), 1);
    std::cout << "Sum: " << futures::reduce(v, 0) << '\n';
    //]

    //[executor Custom executors
    asio::thread_pool custom_pool(4);
    asio::thread_pool::executor_type ex = custom_pool.executor();
    futures::for_each(ex, v.begin(), v.begin() + 10, [](int x) {
        std::cout << x << '\n';
    });
    //]

    //[partitioner Custom partitioner
    auto halve = [](auto first, auto last) {
        return std::next(first, (last - first) / 2);
    };
    auto it = futures::find(ex, halve, v, 3000);
    if (it != v.end()) {
        std::ptrdiff_t pos = it - v.begin();
        std::cout << *it << " found at v[" << pos << "]\n";
    }
    //]

    return 0;
}