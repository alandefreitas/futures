#define FUTURES_DEFAULT_THREAD_POOL_SIZE 1
#include <futures/futures.hpp>
#include <iostream>

int
main() {
    using namespace futures;

    {
        //[std_async Simple std::async task
        std::future<int> f = std::async([]() {
            std::cout << "Parallel work\n";
            return 65;
        });
        std::cout << "Main work\n";
        std::cout << f.get() << '\n';
        //]
    }

    {
        //[wait_for_next Always waiting for next task
        std::future<int> A = std::async([]() { return 65; });

        std::future<char> B = std::async(
            [](int v) { return static_cast<char>(v); },
            A.get());

        std::future<void> C = std::async(
            [](char c) { std::cout << "Result " << c << '\n'; },
            B.get());

        C.wait();
        //]
    }

    {
        //[polling Polling the previous task
        std::future<int> A = std::async([]() { return 65; });

        std::future<char> B = std::async([&A]() {
            return static_cast<char>(A.get());
        });

        std::future<void> C = std::async([&B]() {
            std::cout << "Result " << B.get() << '\n';
        });

        C.wait();
        //]
    }

    {
        //[continuables Continuable futures
        cfuture<int> A = async([]() { return 65; });

        cfuture<char> B = A.then([](int v) {
            return static_cast<char>(v);
        });

        cfuture<void> C = then(B, [](char c) {
            std::cout << "Result " << c << '\n';
        });

        C.wait();
        //]
    }

    {
        //[deferred_continuables Continuable futures
        dcfuture<int> A = schedule([]() { return 65; });

        dcfuture<char> B = then(A, [](int v) { return static_cast<char>(v); });

        dcfuture<void> C = then(B, [](char c) {
            std::cout << "Result " << c << '\n';
        });

        C.wait(); // launch A now!
        //]
    }

    return 0;
}