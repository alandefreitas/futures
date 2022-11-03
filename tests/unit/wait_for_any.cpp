#include <futures/wait_for_any.hpp>
//
#include <futures/is_ready.hpp>
#include <futures/launch.hpp>
#include <catch2/catch.hpp>
#include <future>
#include <type_traits>

TEST_CASE("wait for any") {
    using namespace futures;

    auto check = [](auto launch) {
        constexpr auto enough_time_for_deadlock = std::chrono::milliseconds(20);
        SECTION("Basic Future") {
            SECTION("Ranges") {
                using future_t = std::
                    invoke_result_t<decltype(launch), std::function<int()>>;
                std::vector<future_t> fs;
                fs.emplace_back(launch(std::function<int()>{
                    [t = enough_time_for_deadlock]() {
                    std::this_thread::sleep_for(t);
                    return 2;
                } }));
                fs.emplace_back(launch(std::function<int()>{
                    [t = enough_time_for_deadlock]() {
                    std::this_thread::sleep_for(t);
                    return 3;
                } }));
                fs.emplace_back(launch(std::function<int()>{
                    [t = enough_time_for_deadlock]() {
                    std::this_thread::sleep_for(t);
                    return 4;
                } }));
                SECTION("Iterators") {
                    auto ready_it = wait_for_any(fs.begin(), fs.end());
                    REQUIRE(ready_it != fs.end());
                    REQUIRE(is_ready(*ready_it));
                    REQUIRE(std::any_of(fs.begin(), fs.end(), [](future_t &f) {
                        return is_ready(f);
                    }));
                    int n = ready_it->get();
                    REQUIRE(n >= 2);
                    REQUIRE(n <= 4);
                }
                SECTION("Ranges") {
                    auto ready_it = wait_for_any(fs);
                    REQUIRE(ready_it != fs.end());
                    REQUIRE(is_ready(*ready_it));
                    REQUIRE(std::any_of(fs.begin(), fs.end(), [](future_t &f) {
                        return is_ready(f);
                    }));
                    int n = ready_it->get();
                    REQUIRE(n >= 2);
                    REQUIRE(n <= 4);
                }
            }

            SECTION("Tuple") {
                auto f1 = launch([t = enough_time_for_deadlock]() {
                    std::this_thread::sleep_for(t);
                    return 2;
                });
                auto f2 = launch([t = enough_time_for_deadlock]() {
                    std::this_thread::sleep_for(t);
                    return 3.3;
                });
                auto f3 = launch([t = enough_time_for_deadlock]() {
                    std::this_thread::sleep_for(t);
                    return;
                });
                std::size_t index = wait_for_any(f1, f2, f3);
                REQUIRE(index <= 2);
                REQUIRE((is_ready(f1) || is_ready(f2) || is_ready(f3)));
                switch (index) {
                case 0:
                {
                    int n = f1.get();
                    REQUIRE(n == 2);
                    break;
                }
                case 1:
                {
                    double n = f2.get();
                    REQUIRE(n > 3.0);
                    REQUIRE(n < 3.5);
                    break;
                }
                case 2:
                {
                    f3.get();
                    break;
                }
                default:
                    FAIL("Impossible switch case");
                }
            }
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
