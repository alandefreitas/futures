#include <array>
#include <string>

#include <catch2/catch.hpp>

#include <futures/futures.h>

TEST_CASE("Cancellable future") {
    using namespace futures;
    SECTION("Cancel jfuture<void>") {
        jcfuture<void> f = async([](stop_token s) {
            while (not s.stop_requested()) {
                std::this_thread::sleep_for(std::chrono::milliseconds(20));
            }
        });
        f.request_stop();
        f.wait();
        REQUIRE(is_ready(f));
    }

    SECTION("Cancel jfuture<int>") {
        jcfuture<int> f = async(
            [](stop_token s, std::chrono::milliseconds x) {
                int i = 0;
                do {
                    std::this_thread::sleep_for(x);
                    ++i;
                } while (not s.stop_requested());
                return i;
            },
            std::chrono::milliseconds(20));
        f.request_stop();
        f.wait();
        REQUIRE(is_ready(f));
        int i = f.get();
        REQUIRE(i > 0);
    }

    SECTION("Continue from jfuture<int>") {
        jcfuture<int> f = async(
            [](stop_token s, std::chrono::milliseconds x) {
                int i = 2;
                do {
                    std::this_thread::sleep_for(x);
                    ++i;
                } while (not s.stop_requested());
                return i;
            },
            std::chrono::milliseconds(20));
        stop_source ss = f.get_stop_source();
        stop_token st = f.get_stop_token();
        stop_token sst = ss.get_token();
        REQUIRE(st == sst);
        auto cont = [](int count) { return static_cast<double>(count) * 1.2; };
        jcfuture<double> f2 = then(f, cont);
        REQUIRE_FALSE(f.valid());
        REQUIRE(not is_ready(f2));
        std::this_thread::sleep_for(std::chrono::milliseconds(60));

        SECTION("Stop from source copy") {
            ss.request_stop();
            double t = f2.get();
            REQUIRE(t >= 2.2);
        }

        SECTION("Stop from original source") {
            // the stop state is not invalided in the original future
            f.request_stop();
            double t = f2.get();
            REQUIRE(t >= 2.2);
        }
    }
}
