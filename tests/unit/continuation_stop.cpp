#include <array>
#include <string>

#include <catch2/catch.hpp>

#include <futures/futures.hpp>

TEST_CASE(TEST_CASE_PREFIX "Continuation Stop - Shared stop source") {
    using namespace futures;
    using namespace std::literals;
    jcfuture<int> f1 = async(
        [&](const stop_token &st, int count) {
            while (!st.stop_requested()) {
                std::this_thread::sleep_for(1ms);
                ++count;
            }
            return count;
        },
        10);

    stop_source ss = f1.get_stop_source();
    auto f2 = then(f1, [](int count) { return count * 2; });
    std::this_thread::sleep_for(100ms);
    REQUIRE_FALSE(is_ready(f2));
    ss.request_stop();
    f2.wait();
    REQUIRE(is_ready(f2));
    int final_count = f2.get();
    REQUIRE(final_count >= 10);
}

TEST_CASE(TEST_CASE_PREFIX "Continuation Stop - Independent stop source") {
    using namespace futures;
    using namespace std::literals;
    jcfuture<int> f1 = async(
        [](const stop_token &st, int count) {
            while (!st.stop_requested()) {
                std::this_thread::sleep_for(1ms);
                ++count;
            }
            return count;
        },
        10);
    stop_source f1_ss = f1.get_stop_source();

    jcfuture<int> f2 = then(f1, [](const stop_token &st, int count) {
        while (!st.stop_requested()) {
            std::this_thread::sleep_for(1ms);
            ++count;
        }
        return count;
    });

    std::this_thread::sleep_for(100ms);
    REQUIRE_FALSE(is_ready(f2));
    f2.request_stop(); // <- not inherited from f1
    std::this_thread::sleep_for(100ms);
    REQUIRE_FALSE(is_ready(f2)); // <- has not received results from f1 yet
    f1_ss.request_stop();        // <- internal future was moved but stop source remains valid (and can be copied)
    f2.wait();                   // <- wait for f1 result to arrive at f2
    REQUIRE(is_ready(f2));
    int final_count = f2.get();
    REQUIRE(final_count >= 10);
}
