#include <futures/launch.hpp>
//
#include <catch2/catch.hpp>
#include <array>
#include <string>

TEST_CASE("Launch") {
    using namespace futures;

    STATIC_REQUIRE(!detail::is_copy_constructible_v<cfuture<void>>);
    STATIC_REQUIRE(detail::is_copy_constructible_v<shared_cfuture<void>>);

    SECTION("async") {
        SECTION("Default executor") {
            SECTION("No return") {
                SECTION("No args") {
                    int i = 0;
                    auto r = ::futures::async([&i]() { ++i; });
                    r.wait();
                    REQUIRE(i == 1);
                }

                SECTION("With args") {
                    int i = 0;
                    auto r = ::futures::async([&i](int x) { i = 2 * x; }, 3);
                    r.wait();
                    REQUIRE(i == 6);
                }
            }
            SECTION("With return") {
                SECTION("No args") {
                    auto r = ::futures::async([]() { return 2; });
                    REQUIRE(r.get() == 2);
                }

                SECTION("With args") {
                    auto r = ::futures::async([](int x) { return 2 * x; }, 3);
                    REQUIRE(r.get() == 6);
                }
            }
            SECTION("Non-trivial return") {
                SECTION("No args") {
                    auto r = ::futures::async([]() {
                        return std::string("Hello");
                    });
                    REQUIRE(r.get() == "Hello");
                }

                SECTION("With args") {
                    auto r = ::futures::
                        async([](int c) { return std::string(5, c); }, '_');
                    REQUIRE(r.get() == "_____");
                }
            }
        }

        SECTION("Custom executor") {
            futures::asio::thread_pool pool(2);
            futures::asio::thread_pool::executor_type ex = pool.executor();

            SECTION("No return") {
                SECTION("No args") {
                    int i = 0;
                    auto r = ::futures::async(ex, [&]() { ++i; });
                    r.wait();
                    REQUIRE(i == 1);
                }

                SECTION("With args") {
                    int i = 0;
                    auto r = ::futures::async(
                        ex,
                        [&](int x) { i = 2 * x; },
                        3);
                    r.wait();
                    REQUIRE(i == 6);
                }
            }
            SECTION("With return") {
                SECTION("No args") {
                    auto r = ::futures::async(ex, []() { return 2; });
                    REQUIRE(r.get() == 2);
                }

                SECTION("With args") {
                    auto r = ::futures::async(
                        ex,
                        [](int x) { return 2 * x; },
                        3);
                    REQUIRE(r.get() == 6);
                }
            }
        }
    }

    SECTION("schedule") {
        SECTION("Default executor") {
            SECTION("No return") {
                SECTION("No args") {
                    int i = 0;
                    auto r = ::futures::schedule([&i]() { ++i; });
                    r.wait();
                    REQUIRE(i == 1);
                }

                SECTION("With args") {
                    int i = 0;
                    auto r = ::futures::schedule([&i](int x) { i = 2 * x; }, 3);
                    r.wait();
                    REQUIRE(i == 6);
                }
            }
            SECTION("With return") {
                SECTION("No args") {
                    auto r = ::futures::schedule([]() { return 2; });
                    REQUIRE(r.get() == 2);
                }

                SECTION("With args") {
                    auto r = ::futures::schedule([](int x) { return 2 * x; }, 3);
                    REQUIRE(r.get() == 6);
                }
            }
            SECTION("Non-trivial return") {
                SECTION("No args") {
                    auto r = ::futures::schedule([]() {
                        return std::string("Hello");
                    });
                    REQUIRE(r.get() == "Hello");
                }

                SECTION("With args") {
                    auto r = ::futures::
                        schedule([](int c) { return std::string(5, c); }, '_');
                    REQUIRE(r.get() == "_____");
                }
            }
        }

        SECTION("Custom executor") {
            futures::asio::thread_pool pool(2);
            futures::asio::thread_pool::executor_type ex = pool.executor();

            SECTION("No return") {
                SECTION("No args") {
                    int i = 0;
                    auto r = ::futures::schedule(ex, [&]() { ++i; });
                    r.wait();
                    REQUIRE(i == 1);
                }

                SECTION("With args") {
                    int i = 0;
                    auto r = ::futures::schedule(
                        ex,
                        [&](int x) { i = 2 * x; },
                        3);
                    r.wait();
                    REQUIRE(i == 6);
                }
            }
            SECTION("With return") {
                SECTION("No args") {
                    auto r = ::futures::schedule(ex, []() { return 2; });
                    REQUIRE(r.get() == 2);
                }

                SECTION("With args") {
                    auto r = ::futures::schedule(
                        ex,
                        [](int x) { return 2 * x; },
                        3);
                    REQUIRE(r.get() == 6);
                }
            }
        }
    }
}
