#include <futures/futures.hpp>
#include <array>
#include <string>
#include <catch2/catch.hpp>

TEST_CASE(TEST_CASE_PREFIX "Launch") {
    using namespace futures;

    STATIC_REQUIRE(std::is_copy_constructible_v<cfuture<void>>);

    auto test_launch_function = [](std::string_view name, auto async_fn) {
        SECTION(name.data()) {
            SECTION("Default executor") {
                SECTION("No return") {
                    SECTION("No args") {
                        int i = 0;
                        auto r = async_fn([&i]() { ++i; });
                        r.wait();
                        REQUIRE(i == 1);
                    }

                    SECTION("With args") {
                        int i = 0;
                        auto r = async_fn([&i](int x) { i = 2 * x; }, 3);
                        r.wait();
                        REQUIRE(i == 6);
                    }
                }
                SECTION("With return") {
                    SECTION("No args") {
                        auto r = async_fn([]() { return 2; });
                        REQUIRE(r.get() == 2);
                    }

                    SECTION("With args") {
                        auto r = async_fn([](int x) { return 2 * x; }, 3);
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
                        auto r = async_fn(ex, [&]() { ++i; });
                        r.wait();
                        REQUIRE(i == 1);
                    }

                    SECTION("With args") {
                        int i = 0;
                        auto r = async_fn(
                            ex,
                            [&](int x) { i = 2 * x; },
                            3);
                        r.wait();
                        REQUIRE(i == 6);
                    }
                }
                SECTION("With return") {
                    SECTION("No args") {
                        auto r = async_fn(ex, []() { return 2; });
                        REQUIRE(r.get() == 2);
                    }

                    SECTION("With args") {
                        auto r = async_fn(
                            ex,
                            [](int x) { return 2 * x; },
                            3);
                        REQUIRE(r.get() == 6);
                    }
                }
            }
        }
    };
    using deferred_options = future_options<
        continuable_opt, always_deferred_opt>;
    STATIC_REQUIRE(
        std::is_same_v<
            deferred_options,
            detail::remove_future_option_t<shared_opt, deferred_options>>);
    test_launch_function("Async", [](auto &&...args) {
        return async(std::forward<decltype(args)>(args)...);
    });
    test_launch_function("Schedule", [](auto &&...args) {
        return schedule(std::forward<decltype(args)>(args)...);
    });
}

TEST_CASE(TEST_CASE_PREFIX "Wait for futures") {
    using namespace futures;
    auto test_launch_function = [](std::string_view name, auto async_fn) {
        SECTION(name.data()) {
            SECTION("Await") {
                SECTION("Integer") {
                    auto f = async_fn([]() { return 2; });
                    REQUIRE(await(f) == 2);
                }
                SECTION("Void") {
                    auto f = async_fn([]() { return; });
                    REQUIRE_NOTHROW(await(f));
                }
            }

            SECTION("Wait for all") {
                SECTION("Basic future") {
                    SECTION("Ranges") {
                        using future_t = std::invoke_result_t<
                            decltype(async_fn),
                            std::function<int()>>;
                        std::vector<future_t> fs;
                        fs.emplace_back(async_fn([]() {
                            return 2;
                        }));
                        fs.emplace_back(async_fn([]() {
                            return 3;
                        }));
                        fs.emplace_back(async_fn([]() {
                            return 4;
                        }));
                        SECTION("Iterators") {
                            wait_for_all(fs.begin(), fs.end());
                            REQUIRE(
                                std::all_of(fs.begin(), fs.end(), [](auto &f) {
                                    return is_ready(f);
                                }));
                        }
                        SECTION("Ranges") {
                            wait_for_all(fs);
                            REQUIRE(
                                std::all_of(fs.begin(), fs.end(), [](auto &f) {
                                    return is_ready(f);
                                }));
                        }
                    }

                    SECTION("Tuple") {
                        auto f1 = async_fn([]() { return 2; });
                        auto f2 = async_fn([]() { return 3.3; });
                        auto f3 = async_fn([]() { return; });
                        wait_for_all(f1, f2, f3);
                        REQUIRE(is_ready(f1));
                        REQUIRE(is_ready(f2));
                        REQUIRE(is_ready(f3));
                    }
                }
            }

            SECTION("Wait for any") {
                constexpr auto enough_time_for_deadlock = std::chrono::
                    milliseconds(20);
                SECTION("Basic Future") {
                    SECTION("Ranges") {
                        using future_t = std::invoke_result_t<
                            decltype(async_fn),
                            std::function<int()>>;
                        std::vector<future_t> fs;
                        fs.emplace_back(async_fn(std::function<int()>{
                            [t = enough_time_for_deadlock]() {
                            std::this_thread::sleep_for(t);
                            return 2;
                        } }));
                        fs.emplace_back(async_fn(std::function<int()>{
                            [t = enough_time_for_deadlock]() {
                            std::this_thread::sleep_for(t);
                            return 3;
                        } }));
                        fs.emplace_back(async_fn(std::function<int()>{
                            [t = enough_time_for_deadlock]() {
                            std::this_thread::sleep_for(t);
                            return 4;
                        } }));
                        SECTION("Iterators") {
                            auto ready_it = wait_for_any(fs.begin(), fs.end());
                            REQUIRE(ready_it != fs.end());
                            REQUIRE(is_ready(*ready_it));
                            REQUIRE(
                                std::any_of(fs.begin(), fs.end(), [](auto &f) {
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
                            REQUIRE(
                                std::any_of(fs.begin(), fs.end(), [](auto &f) {
                                    return is_ready(f);
                                }));
                            int n = ready_it->get();
                            REQUIRE(n >= 2);
                            REQUIRE(n <= 4);
                        }
                    }

                    SECTION("Tuple") {
                        auto f1 = async_fn([t = enough_time_for_deadlock]() {
                            std::this_thread::sleep_for(t);
                            return 2;
                        });
                        auto f2 = async_fn([t = enough_time_for_deadlock]() {
                            std::this_thread::sleep_for(t);
                            return 3.3;
                        });
                        auto f3 = async_fn([t = enough_time_for_deadlock]() {
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
                            REQUIRE(false);
                        }
                    }
                }
            }
        }
    };
    test_launch_function("Async", [](auto &&...args) {
        return futures::async(std::forward<decltype(args)>(args)...);
    });
    test_launch_function("std::async", [](auto &&...args) {
        return std::async(std::forward<decltype(args)>(args)...);
    });
    test_launch_function("Schedule", [](auto &&...args) {
        return schedule(std::forward<decltype(args)>(args)...);
    });
}
