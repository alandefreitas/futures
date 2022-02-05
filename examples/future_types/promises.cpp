#include <futures/algorithm.hpp>
#include <futures/futures.hpp>
#include <iostream>

int
main() {
    using namespace futures;

    promise<int> p1;
    auto f1 = p1.get_future();
    p1.set_value(2);
    std::cout << f1.get() << '\n';

    promise<int> p2;
    auto f2 = p2.get_future();
    std::thread t2([&p2]() {
        p2.set_value(2);
    });
    std::cout << f2.get() << '\n';
    t2.join();

    promise<int> p3;
    auto f3 = p3.get_future();
    asio::thread_pool pool(1);
    asio::post(pool, [&p3]() {
        p3.set_value(2);
    });
    std::cout << f3.get() << '\n';

    promise<int, future_options<>> p4;
    vfuture<int> f4 = p4.get_future();
    std::thread t4([&p4]() {
        p4.set_value(2);
    });
    std::cout << f4.get() << '\n';
    t4.join();
}