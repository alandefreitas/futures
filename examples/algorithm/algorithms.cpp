#include <futures/algorithm.hpp>
#include <futures/futures.hpp>
#include <iostream>

int
main() {
    using namespace futures;

    //[algorithm Parallel Algorithms
    std::vector<int> v(50000);
    std::iota(v.begin(), v.end(), 1);

    int c = futures::reduce(v, 0); // parallel by default
    std::cout << "Sum: " << c << '\n';
    //]

    //[executor Custom executor
    asio::thread_pool pool(4);
    auto ex = pool.executor();
    futures::for_each(ex, v.begin(), v.begin() + 10, [](int x) {
        std::cout << x << '\n';
    });
    //]

    //[partitioner Defining a custom partitioner
    auto p = [](auto first, auto last) {
        return std::next(first, (last - first) / 2);
    };
    //]
    //[partitioner_algorithm Using a custom partitioner
    auto it = find(ex, p, v, 3000);
    if (it != v.end()) {
        std::ptrdiff_t pos = it - v.begin();
        std::cout << *it << " found at v[" << pos << "]\n";
    }
    //]

    return 0;
}