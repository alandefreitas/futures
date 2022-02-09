#include <futures/algorithm.hpp>
#include <futures/futures.hpp>
#include <iostream>

int
main() {
    using namespace futures;

    //[std_async C++ std::async
    std::future<void> future1 = std::async([] {
        std::cout << "std::async task\n";
    });
    //]

    //[cfuture Launching a continuable future
    cfuture<void> future2 = futures::async([] {
        std::cout << "continuable task\n";
    });
    //]

    //[jcfuture Launching a stoppable future
    jcfuture<void> future3 = futures::async([](stop_token st) {
        int a = 0;
        while (!st.stop_requested()) {
            ++a;
        }
        std::cout << "task stopped" << '\n';
    });
    //]

    //[jcfuture_stop Requesting task to stop
    future3.request_stop();
    //]

    //[wait_for_all All of these types interoperate!
    wait_for_all(future1, future2, future3);
    //]
}