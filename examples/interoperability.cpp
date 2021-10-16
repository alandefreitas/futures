#include <futures/algorithm.h>
#include <futures/futures.h>
#include <iostream>

int main() {
    using namespace futures;

    std::future<void> future1 = std::async([] { std::cout << "std::async task" << std::endl; });

    cfuture<void> future2 = futures::async([] { std::cout << "continuable task" << std::endl; });

    jcfuture<void> future3 = futures::async([] (stop_token st) {
        int a = 0;
        while (!st.stop_requested()) {
            ++a;
        }
        std::cout << "task stopped" << std::endl; }
    );
    future3.request_stop();

    // All these types can interoperate!
    auto all = when_all(future1, future2, future3);
    all.wait();
}