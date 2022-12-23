#include <futures/wait_for_all.hpp>
//
#include <futures/is_ready.hpp>
#include <futures/launch.hpp>
#include <catch2/catch.hpp>
#include <future>

TEST_CASE("wait for all") {
    using namespace futures;

    auto check = [](auto launch) {
        SECTION("Ranges") {
            using future_t = detail::
                invoke_result_t<decltype(launch), std::function<int()>>;
            std::vector<future_t> fs;
            fs.emplace_back(launch(std::function<int()>([]() { return 2; })));
            fs.emplace_back(launch(std::function<int()>([]() { return 3; })));
            fs.emplace_back(launch(std::function<int()>([]() { return 4; })));
            SECTION("Iterators") {
                wait_for_all(fs.begin(), fs.end());
                REQUIRE(std::all_of(fs.begin(), fs.end(), [](future_t &f) {
                    return is_ready(f);
                }));
            }
            SECTION("Ranges") {
                wait_for_all(fs);
                REQUIRE(std::all_of(fs.begin(), fs.end(), [](future_t &f) {
                    return is_ready(f);
                }));
            }
        }

        SECTION("Tuple") {
            auto f1 = launch([]() { return 2; });
            auto f2 = launch([]() { return 3.3; });
            auto f3 = launch([]() { return; });
            wait_for_all(f1, f2, f3);
            REQUIRE(is_ready(f1));
            REQUIRE(is_ready(f2));
            REQUIRE(is_ready(f3));
        }
    };

    SECTION("Async") {
        check([](auto &&...args) {
            return futures::async(std::forward<decltype(args)>(args)...);
        });
    }
    SECTION("std::async") {
        check([](auto &&...args) {
            return std::async(std::forward<decltype(args)>(args)...);
        });
    }
    SECTION("Schedule") {
        check([](auto &&...args) {
            return schedule(std::forward<decltype(args)>(args)...);
        });
    }
}
