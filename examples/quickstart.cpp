#include <futures/algorithm.h>
#include <futures/futures.h>
#include <iostream>

int main() {
    using namespace futures;

    /*
     * Launching tasks
     */

    // futures::async uses asio executors instead of launching new threads
    auto task1 = futures::async([] { std::cout << "Task 1 in default executor. A thread pool." << std::endl; });

    // a std::future return by std::async works with futures future types because it respects the future concept
    auto task2 = std::async([] { std::cout << "Task 2 in a new thread." << std::endl; });

    // futures::async can use custom executors
    asio::thread_pool custom_pool(1);
    asio::thread_pool::executor_type ex = custom_pool.executor();
    auto task3 = futures::async(ex, [] { std::cout << "Task 3 in a custom executor." << std::endl; });

    // futures::async can launch stoppable tasks
    auto task4 = futures::async(ex, [] (futures::stop_token st) {
        int a = 0;
        while (!st.stop_requested()) {
            ++a;
        }
        std::cout << "Task 4 had a stop token" << std::endl;
    });

    /*
     * Continuations
     */

    // task6 continues with the result of task5
    auto task5 = futures::async([]() -> int { return 42; });
    auto task6 = futures::then(task5, [](int x) { return x * 2; });

    // task8 will continues with the result of task7 even though task7 is a std::future
    auto task7 = std::async([]() -> int { return 63; });

    // operator>> for continuations
    auto task8 = task7 >> [](int x) { return x * 2; };

    /*
     * Conjunctions
     */

    // futures::when_all can accept any future type that respects the future concept
    task4.request_stop();
    auto task9 = futures::when_all(task1, task2, task3, task4);
    task9.wait();

    // operator&& for tasks
    auto task10 = task6 && task8;

    // futures::then can unwrap the results of when_all in a variety of ways
    auto task11 = futures::then(task10, [](int a, int b) { return a * b; });

    /*
     * Disjunctions
     */

    // futures::when_all can accept any future type that respects the future concept
    auto task12 = futures::async([]() -> int { return 10; });
    auto task13 = futures::async([]() -> int { return 11; });
    auto task14 = futures::when_any(task11, task12, task13);

    // operator|| for tasks
    auto task15 = futures::async([]() -> int { return 15; });
    auto task16 = futures::async([]() -> int { return 16; });
    auto task17 = task15 || task16;

    // futures::then can unwrap the results of when_all in a variety of ways
    auto task18 = futures::then(task14, [](int first) { std::cout << first << std::endl; });
    task18.wait();
    std::cout << "Task 18 has completed" << std::endl;
    task17.wait();

    /*
     * Parallel algorithms
     */
    std::vector<int> v(50000);
    std::iota(v.begin(), v.end(), 1);

    // Algorithms in futures are parallel by default. No need for TBB. Just asio.
    int c = futures::reduce(v, 0);
    std::cout << "Sum: " << c << std::endl;

    // The launch policy can be replaced with a asio custom executor
    asio::thread_pool custom_pool2(4);
    asio::thread_pool::executor_type ex2 = custom_pool2.executor();
    futures::for_each(ex2, v.begin(), v.begin() + 10, [](int x) {
        std::cout << x << std::endl;
    });
}