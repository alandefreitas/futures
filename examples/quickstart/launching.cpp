#include <futures/algorithm.hpp>
#include <futures/futures.hpp>
#include <iostream>

int
main() {
    using namespace futures;

    //[launching Basic Usage
    cfuture<void> f1 = async([] {
        std::cout << "Task 1 in default executor. A thread pool.\n";
    });

    // analogous to:
    std::future<void> f2 = std::async([] {
        std::cout << "Task 2 in a new thread from std::async.\n";
    });
    //]

    //[executor Custom executor
    asio::thread_pool custom_pool(1);
    asio::thread_pool::executor_type ex = custom_pool.executor();
    auto f3 = async(ex, [] {
        std::cout << "Task 3 in a custom executor.\n";
    });
    //]

    //[stoppable Stop token
    auto f4 = async(ex, [](stop_token st) {
        int a = 0;
        while (!st.stop_requested()) {
            ++a;
        }
        std::cout << "Task 4 stopped when a = " << a << '\n';
    });
    // ...
    f4.request_stop();
    //]

    //[deferred Deferred sender
    auto f5 = schedule([] {
        std::cout << "Deferred task.\n";
    });
    //]

    //[interop Interoperation
    wait_for_all(f1, f2, f3, f4, f5);
    //]

    return 0;
}