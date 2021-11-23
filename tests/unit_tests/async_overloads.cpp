#include <array>
#include <string>

#include <catch2/catch.hpp>

#include <futures/futures.h>

TEST_CASE("Async overloads") {
    using namespace futures;
    SECTION("Default executor") {
        SECTION("No return") {
            SECTION("No args") {
                int i = 0;
                cfuture<void> r = async([&]() { ++i; });
                r.wait();
                REQUIRE(i == 1);
            }

            SECTION("With args") {
                int i = 0;
                cfuture<void> r = async([&](int x) { i = 2 * x; }, 3);

                r.wait();
                REQUIRE(i == 6);
            }
        }
        SECTION("With return") {
            SECTION("No args") {
                cfuture<int> r = async([]() { return 2; });
                REQUIRE(r.get() == 2);
            }

            SECTION("With args") {
                cfuture<int> r = async([](int x) { return 2 * x; }, 3);
                REQUIRE(r.get() == 6);
            }
        }
    }

    SECTION("Custom executor") {
        asio::thread_pool pool(2);
        asio::thread_pool::executor_type ex = pool.executor();

        SECTION("No return") {
            SECTION("No args") {
                int i = 0;
                cfuture<void> r = async(ex, [&]() { ++i; });
                r.wait();
                REQUIRE(i == 1);
            }

            SECTION("With args") {
                int i = 0;
                cfuture<void> r = async(
                    ex, [&](int x) { i = 2 * x; }, 3);
                r.wait();
                REQUIRE(i == 6);
            }
        }
        SECTION("With return") {
            SECTION("No args") {
                cfuture<int> r = async(ex, []() { return 2; });
                REQUIRE(r.get() == 2);
            }

            SECTION("With args") {
                cfuture<int> r = async(
                    ex, [](int x) { return 2 * x; }, 3);
                REQUIRE(r.get() == 6);
            }
        }
    }

    SECTION("Precedence") {
        SECTION("Now") {
            auto f = async([] {
                int i = 0;
                auto f = async(launch::executor_now, [&] { i = 1; });
                REQUIRE(i == 1);
                f.wait();
                return 2;
            });
            REQUIRE(f.get() == 2);
        }

        SECTION("Later") {
            cfuture<void> f2;
            auto f = async([&f2] {
                int i = 0;
                f2 = async(launch::executor_later, [&] { i = 1; });
                REQUIRE(i == 0);
                REQUIRE_FALSE(is_ready(f2));
                return 2;
            });
            REQUIRE(f.get() == 2);
        }
    }
}