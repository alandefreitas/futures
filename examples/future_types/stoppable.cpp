#include <futures/algorithm.hpp>
#include <futures/futures.hpp>
#include <iostream>

int
main() {
    using namespace futures;

    jcfuture<void> f = async([](stop_token s) {
        while (!s.stop_requested()) {
            std::this_thread::sleep_for(
                std::chrono::milliseconds(20));
        }
    });

    // It won't be ready until we ask it to stop
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    std::cout << is_ready(f) << std::endl;

    // Request stop
    f.request_stop();
    f.wait();
    std::cout << is_ready(f) << std::endl;

    return 0;
}