#include <futures/futures.hpp>
#include <array>
#include <string>
#include <catch2/catch.hpp>

TEST_CASE(TEST_CASE_PREFIX "Cancellable future") {
    using namespace futures;
    SECTION("Cancel jfuture<void>") {
        jcfuture<void> f = async([](stop_token s) {
            while (!s.stop_requested()) {
                std::this_thread::sleep_for(std::chrono::milliseconds(20));
            }
        });
        f.request_stop();
        f.wait();
        REQUIRE(is_ready(f));
    }

    SECTION("Cancel jfuture<int>") {
        auto fn = [](const stop_token& s, std::chrono::milliseconds x) {
            int32_t i = 0;
            do {
                std::this_thread::sleep_for(x);
                ++i;
            }
            while (!s.stop_requested());
            return i;
        };
        jcfuture<int32_t> f = async(fn, std::chrono::milliseconds(20));
        f.request_stop();
        f.wait();
        REQUIRE(is_ready(f));
        int32_t i = f.get();
        REQUIRE(i > 0);
    }

    SECTION("Continue from jfuture<int>") {
        jcfuture<int32_t> f = async(
            [](stop_token s, std::chrono::milliseconds x) {
            int32_t i = 2;
            do {
                std::this_thread::sleep_for(x);
                ++i;
            }
            while (!s.stop_requested());
            return i;
            },
            std::chrono::milliseconds(20));

        stop_source ss = f.get_stop_source();
        stop_token st = f.get_stop_token();
        stop_token sst = ss.get_token();
        REQUIRE(st == sst);

        auto cont = [](int32_t count) {
            return static_cast<double>(count) * 1.2;
        };

        SECTION("Standalone then") {
            auto f2 = then(f, cont);

            REQUIRE(!is_ready(f2));

            std::this_thread::sleep_for(std::chrono::milliseconds(60));
            ss.request_stop();

            double t = f2.get();
            REQUIRE(t >= 2.2);
        }

        SECTION("Member then") {
            cfuture<double> f2 = f.then(cont);

            REQUIRE(!is_ready(f2));

            std::this_thread::sleep_for(std::chrono::milliseconds(60));
            ss.request_stop();

            double t = f2.get();
            REQUIRE(t >= 2.2);
        }
    }
}
