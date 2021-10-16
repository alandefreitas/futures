#include <futures/algorithm.h>
#include <futures/futures.h>
#include <iostream>

int main() {
    using namespace futures;

    /*
     * Launching futures
     */

    // futures::async uses asio executors instead of launching new threads
    auto future1 = futures::async([] { std::cout << "Task 1 in default executor. A thread pool." << std::endl; });

    // the std::future returned by std::async works with other future types
    // because it has the requirements of the `is_future` concept
    auto future2 = std::async([] { std::cout << "Task 2 in a new thread." << std::endl; });

    // futures::async can launch futures with custom executors
    asio::thread_pool custom_pool(1);
    asio::thread_pool::executor_type ex = custom_pool.executor();
    auto future3 = futures::async(ex, [] { std::cout << "Task 3 in a custom executor." << std::endl; });

    // futures::async can launch stoppable futures
    auto future4 = futures::async(ex, [](futures::stop_token st) {
        int a = 0;
        while (!st.stop_requested()) {
            ++a;
        }
        std::cout << "Task 4 had a stop token. It stopped when a = " << a << std::endl;
    });

    /*
     * Continuations
     */

    // future6 continues with the result of future5
    auto future5 = futures::async([]() -> int { return 42; });
    auto future6 = futures::then(future5, [](int x) { return x * 2; });

    // future8 will continue with the result of future7 even though future7 is a std::future
    auto future7 = std::async([]() -> int { return 63; });

    // We can use operator>> for continuations
    auto future8 = future7 >> [](int x) { return x * 2; };

    /*
     * Conjunctions
     */

    // futures::when_all can accept any future type that has the requirements of the future concept
    future4.request_stop();
    auto future9 = futures::when_all(future1, future2, future3, future4);
    future9.wait();

    // operator&& can be used for future conjunction
    auto future10 = future6 && future8;

    // futures::then can unwrap the results of when_all in a variety of ways
    auto future11 = futures::then(future10, [](int a, int b) { return a * b; });

    /*
     * Disjunctions
     */

    // futures::when_all can accept any future type that has the requirements of the future concept
    auto future12 = futures::async([]() -> int { return 10; });
    auto future13 = futures::async([]() -> int { return 11; });
    auto future14 = futures::when_any(future11, future12, future13);

    // operator|| can be used for future disjunctions
    auto future15 = futures::async([]() -> int { return 15; });
    auto future16 = futures::async([]() -> int { return 16; });
    auto future17 = future15 || future16;

    // futures::then can unwrap the results of when_all in a variety of ways
    auto future18 = futures::then(future14, [](int first) { std::cout << first << std::endl; });
    future18.wait();
    std::cout << "Task 18 has completed" << std::endl;
    future17.wait();

    /*
     * Parallel algorithms
     */
    std::vector<int> v(50000);
    std::iota(v.begin(), v.end(), 1);

    // Algorithms in futures:: are parallel by default. No need for TBB. Just asio executors.
    int c = futures::reduce(v, 0);
    std::cout << "Sum: " << c << std::endl;

    // The launch policy can be replaced with an asio custom executor
    asio::thread_pool custom_pool2(4);
    asio::thread_pool::executor_type ex2 = custom_pool2.executor();
    futures::for_each(ex2, v.begin(), v.begin() + 10, [](int x) { std::cout << x << std::endl; });
}