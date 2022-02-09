#include <futures/algorithm.hpp>
#include <futures/futures.hpp>
#include <iostream>

int
main() {
    using namespace futures;

    //[inline Inline packaged task
    packaged_task<int()> p1([]() {
        return 2;
    });
    auto f1 = p1.get_future();
    p1();
    std::cout << f1.get() << '\n';
    //]

    //[thread Packaged task invoked by thread
    packaged_task<int()> p2([]() {
        return 2;
    });
    auto f2 = p2.get_future();
    std::thread t(std::move(p2));
    std::cout << f2.get() << '\n';
    t.join();
    //]

    //[executor Packaged task invoked by executor
    packaged_task<int()> p3([]() {
        return 2;
    });
    auto f3 = p3.get_future();
    asio::thread_pool pool(1);
    asio::post(pool, std::move(p3));
    std::cout << f3.get() << '\n';
    //]
}