#include <futures/algorithm.h>
#include <futures/futures.h>
#include <iostream>

int main() {
    using namespace futures;

    std::vector<int> v(50000);
    std::iota(v.begin(), v.end(), 1);
    std::cout << "Sum: " << futures::reduce(v, 0) << std::endl;

    asio::thread_pool custom_pool(4);
    asio::thread_pool::executor_type ex = custom_pool.executor();
    futures::for_each(ex, v.begin(), v.begin() + 10,
                      [](int x) { std::cout << x << std::endl; });

    return 0;
}