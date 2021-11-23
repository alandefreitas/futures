#include <array>
#include <string>

#include <catch2/catch.hpp>

#include <futures/futures.h>

TEST_CASE("Conjunction") {
    using namespace futures;
    SECTION("Empty conjunction") {
        auto f = when_all();
        REQUIRE(f.valid());
        REQUIRE_NOTHROW(f.wait());
        REQUIRE(f.wait_for(std::chrono::seconds(0)) == std::future_status::ready);
        REQUIRE(f.wait_until(std::chrono::system_clock::now() + std::chrono::seconds(0)) == std::future_status::ready);
        REQUIRE(is_ready(f));
        REQUIRE(f.get() == std::make_tuple());
    }

    SECTION("Tuple conjunction") {
        auto f1 = async([]() { return 2; });
        auto f2 = async([]() { return 3.5; });
        auto f3 = async([]() -> std::string { return "name"; });
        auto f = when_all(f1, f2, f3);
        REQUIRE(f.valid());
        REQUIRE_FALSE(f1.valid());
        REQUIRE_FALSE(f2.valid());
        REQUIRE_FALSE(f3.valid());

        SECTION("Wait") {
            REQUIRE_NOTHROW(f.wait());
            REQUIRE_NOTHROW(f.wait_for(std::chrono::seconds(0)) == std::future_status::ready);
            REQUIRE_NOTHROW(f.wait_until(std::chrono::system_clock::now() + std::chrono::seconds(0)) ==
                            std::future_status::ready);
            REQUIRE(is_ready(f));
            auto [r1, r2, r3] = f.get();
            REQUIRE(r1.get() == 2);
            double d = r2.get();
            REQUIRE(d >= 3.0);
            REQUIRE(d <= 4.0);
            REQUIRE(r3.get() == "name");
        }

        SECTION("Continue") {
            auto continuation = [](std::tuple<cfuture<int>, cfuture<double>, cfuture<std::string>> r) {
                return std::get<0>(r).get() + static_cast<int>(std::get<1>(r).get()) + std::get<2>(r).get().size();
            };
            STATIC_REQUIRE(is_future_v<decltype(f)>);
            STATIC_REQUIRE(is_future_continuation_v<decltype(continuation), decltype(f)>);
            auto f4 = then(f, continuation);
            REQUIRE(f4.get() == 2 + 3 + 4);
        }

        SECTION("Unwrap to futures") {
            auto f4 = then(f, [](cfuture<int> r1, cfuture<double> r2, cfuture<std::string> r3) {
                return r1.get() + static_cast<int>(r2.get()) + r3.get().size();
            });
            REQUIRE(f4.get() == 2 + 3 + 4);
        }

        SECTION("Unwrap to values") {
            auto f4 =
                then(f, [](int r1, double r2, const std::string &r3) { return r1 + static_cast<int>(r2) + r3.size(); });
            REQUIRE(f4.get() == 2 + 3 + 4);
        }
    }

    SECTION("Tuple conjunction with lambdas") {
        auto f1 = async([]() { return 2; });
        auto f2 = []() { return 3.5; };
        REQUIRE(f1.valid());
        auto f = when_all(f1, f2);
        REQUIRE(f.valid());
        REQUIRE_FALSE(f1.valid());
        auto [r1, r2] = f.get();
        REQUIRE(r1.get() == 2);
        double d = r2.get();
        REQUIRE(d > 3.);
        REQUIRE(d < 4.);
    }

    SECTION("Range conjunction") {
        std::vector<cfuture<int>> range;
        range.emplace_back(async([]() { return 2; }));
        range.emplace_back(async([]() { return 3; }));
        range.emplace_back(async([]() { return 4; }));
        auto f = when_all(range);
        REQUIRE(f.valid());
        REQUIRE_FALSE(range[0].valid());
        REQUIRE_FALSE(range[1].valid());
        REQUIRE_FALSE(range[2].valid());

        SECTION("Wait") {
            REQUIRE_NOTHROW(f.wait());
            REQUIRE_NOTHROW(f.wait_for(std::chrono::seconds(0)) == std::future_status::ready);
            REQUIRE_NOTHROW(f.wait_until(std::chrono::system_clock::now() + std::chrono::seconds(0)) ==
                            std::future_status::ready);
            REQUIRE(is_ready(f));
            auto rs = f.get();
            REQUIRE(rs[0].get() == 2);
            REQUIRE(rs[1].get() == 3);
            REQUIRE(rs[2].get() == 4);
        }

        SECTION("No unwrapping") {
            SECTION("Continue with value") {
                auto continuation = [](futures::small_vector<cfuture<int>> rs) {
                    return rs[0].get() + rs[1].get() + rs[2].get();
                };
                STATIC_REQUIRE(is_future_v<decltype(f)>);
                STATIC_REQUIRE(is_future_continuation_v<decltype(continuation), decltype(f)>);
                auto f4 = then(f, continuation);
                REQUIRE(f4.get() == 2 + 3 + 4);
            }

            SECTION("Continue with lvalue") {
                auto continuation = [](futures::small_vector<cfuture<int>> &rs) {
                    return rs[0].get() + rs[1].get() + rs[2].get();
                };
                STATIC_REQUIRE(is_future_v<decltype(f)>);
                STATIC_REQUIRE(not is_future_v<decltype(continuation)>);
                STATIC_REQUIRE(is_future_continuation_v<decltype(continuation), decltype(f)>);
                auto f4 = then(f, continuation);
                REQUIRE(f4.get() == 2 + 3 + 4);
            }

            SECTION("Continue with const lvalue") {
                auto continuation = [](const futures::small_vector<cfuture<int>> &) {
                    return 2 + 3 + 4; // <- cannot get from const future :P
                };
                STATIC_REQUIRE(is_future_v<decltype(f)>);
                STATIC_REQUIRE(not is_future_v<decltype(continuation)>);
                STATIC_REQUIRE(is_future_continuation_v<decltype(continuation), decltype(f)>);
                auto f4 = then(f, continuation);
                REQUIRE(f4.get() == 2 + 3 + 4);
            }

            SECTION("Continue with rvalue") {
                auto continuation = [](futures::small_vector<cfuture<int>> &&rs) {
                    return rs[0].get() + rs[1].get() + rs[2].get();
                };
                STATIC_REQUIRE(is_future_v<decltype(f)>);
                STATIC_REQUIRE(not is_future_v<decltype(continuation)>);
                STATIC_REQUIRE(is_future_continuation_v<decltype(continuation), decltype(f)>);
                auto f4 = then(f, continuation);
                REQUIRE(f4.get() == 2 + 3 + 4);
            }
        }

        SECTION("Unwrap vector") {
            SECTION("Continue with value") {
                auto continuation = [](futures::small_vector<int> rs) { return rs[0] + rs[1] + rs[2]; };
                STATIC_REQUIRE(is_future_v<decltype(f)>);
                STATIC_REQUIRE(is_future_continuation_v<decltype(continuation), decltype(f)>);
                auto f4 = then(f, continuation);
                REQUIRE(f4.get() == 2 + 3 + 4);
            }

            SECTION("Continue with lvalue") {
                auto continuation = [](futures::small_vector<int> &rs) { return rs[0] + rs[1] + rs[2]; };
                STATIC_REQUIRE(is_future_v<decltype(f)>);
                STATIC_REQUIRE(is_future_continuation_v<decltype(continuation), decltype(f)>);
                auto f4 = then(f, continuation);
                REQUIRE(f4.get() == 2 + 3 + 4);
            }

            SECTION("Continue with const lvalue") {
                auto continuation = [](const futures::small_vector<int> &rs) { return rs[0] + rs[1] + rs[2]; };
                STATIC_REQUIRE(is_future_v<decltype(f)>);
                STATIC_REQUIRE(is_future_continuation_v<decltype(continuation), decltype(f)>);
                auto f4 = then(f, continuation);
                REQUIRE(f4.get() == 2 + 3 + 4);
            }
        }
    }

    SECTION("Range conjunction with lambdas") {
        std::function<int()> f1 = []() { return 2; };
        std::function<int()> f2 = []() { return 3; };
        std::vector<std::function<int()>> range;
        range.emplace_back(f1);
        range.emplace_back(f2);
        auto f = when_all(range);
        REQUIRE(f.valid());
        auto rs = f.get();
        REQUIRE(rs[0].get() == 2);
        REQUIRE(rs[1].get() == 3);
    }

    SECTION("Operator&&") {
        SECTION("Future conjunction") {
            cfuture<int> f1 = async([] { return 1; });
            cfuture<int> f2 = async([] { return 2; });
            auto f = f1 && f2;
            REQUIRE_FALSE(f1.valid());
            REQUIRE_FALSE(f2.valid());
            auto [r1, r2] = f.get();
            REQUIRE(r1.get() == 1);
            REQUIRE(r2.get() == 2);
        }

        SECTION("Lambda conjunction") {
            // When a lambda is passed to when_all, it is converted into a future immediately
            auto f = [] { return 1; } && [] { return 2; };
            auto [r1, r2] = f.get();
            REQUIRE(r1.get() == 1);
            REQUIRE(r2.get() == 2);
        }

        SECTION("Future/lambda conjunction") {
            cfuture<int> f1 = async([] { return 1; });
            auto f = f1 && [] { return 2; };
            REQUIRE_FALSE(f1.valid());
            auto [r1, r2] = f.get();
            REQUIRE(r1.get() == 1);
            REQUIRE(r2.get() == 2);
        }

        SECTION("Lambda/future conjunction") {
            cfuture<int> f2 = async([] { return 2; });
            auto f = [] { return 1; } && f2;
            REQUIRE_FALSE(f2.valid());
            auto [r1, r2] = f.get();
            REQUIRE(r1.get() == 1);
            REQUIRE(r2.get() == 2);
        }

        SECTION("Concatenate conjunctions") {
            cfuture<int> f1 = async([] { return 1; });
            cfuture<int> f2 = async([] { return 2; });
            cfuture<int> f3 = async([] { return 3; });
            auto f = f1 && f2 && f3 && []() { return 4; };
            REQUIRE_FALSE(f1.valid());
            REQUIRE_FALSE(f2.valid());
            REQUIRE_FALSE(f3.valid());
            auto [r1, r2, r3, r4] = f.get();
            REQUIRE(r1.get() == 1);
            REQUIRE(r2.get() == 2);
            REQUIRE(r3.get() == 3);
            REQUIRE(r4.get() == 4);
        }
    }

    SECTION("Conjunction continuation") {
        cfuture<int> f1 = async([] { return 1; });
        cfuture<int> f2 = async([] { return 2; });
        cfuture<int> f3 = async([] { return 3; });
        auto f = f1 && f2 && f3 && []() { return 4; };
        auto c = f >> [](int a, int b, int c, int d) { return a + b + c + d; } >> [](int s) { return s * 2; };
        REQUIRE(c.get() == (1 + 2 + 3 + 4) * 2);
    }
}
