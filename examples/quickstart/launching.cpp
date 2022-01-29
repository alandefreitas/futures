#include <futures/algorithm.hpp>
#include <futures/futures.hpp>
#include <iostream>

int main() {
    using namespace futures;

    auto f1 = futures::async([] {
        std::cout << "Task 1 in default executor. A thread pool."
                  << '\n';
    });

    auto f2 = std::async([] {
        std::cout << "Task 2 in a new thread from std::async."
                  << '\n';
    });

    asio::thread_pool custom_pool(1);
    asio::thread_pool::executor_type ex = custom_pool.executor();
    auto f3 = futures::async(ex, [] {
        std::cout << "Task 3 in a custom executor." << '\n';
    });

    auto f4 = futures::async(ex, [](futures::stop_token st) {
        int a = 0;
        while (!st.stop_requested()) {
            ++a;
        }
        std::cout << "Task 4 had a stop token. It stopped when a = "
                  << a << '\n';
    });

    f1.wait();
    f2.wait();
    f3.wait();
    f4.request_stop();
    f4.wait();

    return 0;
}