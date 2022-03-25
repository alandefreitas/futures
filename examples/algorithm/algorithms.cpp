#include <futures/algorithm.hpp>
#include <futures/futures.hpp>
#include <array>
#include <iostream>
#include <numeric>
#include <vector>

int
main() {
    using namespace futures;

    std::vector<int> v(50000);
    std::iota(v.begin(), v.end(), 1);

    {
        //[algorithm Parallel Algorithms
        int c = futures::reduce(v.begin(), v.end(), 0); // parallel by default
        std::cout << "Sum: " << c << '\n';
        //]
    }

    {
        //[algorithm_range Ranges
        int c = futures::reduce(v, 0); // parallel by default
        std::cout << "Sum: " << c << '\n';
        //]
    }

    //[executor Custom executor
    asio::thread_pool pool(4);
    auto ex = pool.executor();
    futures::for_each(ex, v.begin(), v.begin() + 10, [](int x) {
        std::cout << x << '\n';
    });
    //]

    {
        //[seq_policy Execution policy
        int c = futures::reduce(futures::seq, v, 0); // sequential execution
        std::cout << "Sum: " << c << '\n';
        //]
    }

    {
        //[inline_executor Inline executor
        int c = futures::reduce(make_inline_executor(), v, 0); // sequential
                                                               // execution
        std::cout << "Sum: " << c << '\n';
        //]
    }

    {
        //[constexpr Compile-time algorithms
        constexpr std::array<int, 5> a = { 1, 2, 3, 4, 5 };
        constexpr int n = futures::reduce(a);
        constexpr std::array<int, n> b{};
        std::cout << "n: " << b.size() << '\n';
        //]
    }

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