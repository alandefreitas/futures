#include <futures/algorithm.hpp>
#include <futures/futures.hpp>
#include <iostream>

int
main() {
    using namespace futures;

    //[launching Launching Futures
    auto f1 = async([] {
        std::cout << "Task 1 in default executor. A thread pool.\n";
    });

    auto f2 = std::async([] {
        std::cout << "Task 2 in a new thread from std::async.\n";
    });

    asio::thread_pool custom_pool(1);
    asio::thread_pool::executor_type ex = custom_pool.executor();
    auto f3 = async(ex, [] {
        std::cout << "Task 3 in a custom executor.\n";
    });

    auto f4 = async(ex, [](stop_token st) {
        int a = 0;
        while (!st.stop_requested()) {
            ++a;
        }
        std::cout << "Task 4 stopped when a = " << a << '\n';
    });

    f1.wait();
    f2.wait();
    f3.wait();
    f4.request_stop();
    f4.wait();
    //]

    return 0;
}