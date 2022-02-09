#include <futures/algorithm.hpp>
#include <futures/futures.hpp>
#include <futures/executor/new_thread_executor.hpp>
#include <iostream>

int
main() {
    using namespace futures;

    //[inline Inline promise
    promise<int> p1;
    cfuture<int> f1 = p1.get_future();
    p1.set_value(2);
    std::cout << f1.get() << '\n';
    //]

    //[thread Promise set by new thread
    promise<int> p2;
    cfuture<int> f2 = p2.get_future();
    std::thread t2([&p2]() {
        p2.set_value(2);
    });
    std::cout << f2.get() << '\n';
    t2.join();
    //]

    //[thread-executor Promise set by new thread executor
    auto f3 = async(make_new_thread_executor(), []() {
        return 2;
    });
    std::cout << f3.get() << '\n';
    //]

    //[thread_pool Promise set by thread pool
    promise<int> p4;
    cfuture<int> f4 = p4.get_future();
    asio::thread_pool pool(1);
    asio::post(pool, [&p4]() {
        p4.set_value(2);
    });
    std::cout << f4.get() << '\n';
    //]

    //[custom_options Promise with custom options
    promise<int, future_options<>> p5;
    vfuture<int> f5 = p5.get_future();
    std::thread t5([&p5]() {
        p5.set_value(2);
    });
    std::cout << f5.get() << '\n';
    t5.join();
    //]
}