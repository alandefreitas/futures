#include <futures/algorithm.hpp>
#include <futures/futures.hpp>
#include <iostream>

int
main() {
    using namespace futures;

    //[stoppable Stoppable task
    jcfuture<void> f = async([](stop_token s) {
        while (!s.stop_requested()) {
            std::this_thread::sleep_for(
                std::chrono::milliseconds(20));
        }
    });
    //]

    //[not_ready
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    std::cout << is_ready(f) << '\n'; // return false
    //]

    //[request_stop Requesting task to stop
    f.request_stop();
    f.wait();
    std::cout << is_ready(f) << '\n'; // return true
    //]

    return 0;
}