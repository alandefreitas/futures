#include <array>
#include <string>

#include <catch2/catch.hpp>
#include <futures/futures.h>

TEST_CASE(TEST_CASE_PREFIX "Async overloads") {
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

TEST_CASE(TEST_CASE_PREFIX "Wait for futures") {
    using namespace futures;

    SECTION("Await") {
        SECTION("Integer") {
            auto f = futures::async([]() { return 2; });
            REQUIRE(await(f) == 2);
        }
        SECTION("Void") {
            auto f = futures::async([]() { return; });
            REQUIRE_NOTHROW(await(f));
        }
    }

    SECTION("Wait for all") {
        SECTION("Basic future") {
            SECTION("Ranges") {
                std::vector<cfuture<int>> fs;
                fs.emplace_back(futures::async([]() { return 2; }));
                fs.emplace_back(futures::async([]() { return 3; }));
                fs.emplace_back(futures::async([]() { return 4; }));
                SECTION("Iterators") {
                    wait_for_all(fs.begin(), fs.end());
                    REQUIRE(std::all_of(fs.begin(), fs.end(), [](auto &f) { return f.is_ready(); }));
                }
                SECTION("Ranges") {
                    wait_for_all(fs);
                    REQUIRE(std::all_of(fs.begin(), fs.end(), [](auto &f) { return f.is_ready(); }));
                }
            }

            SECTION("Tuple") {
                auto f1 = futures::async([]() { return 2; });
                auto f2 = futures::async([]() { return 3.3; });
                auto f3 = futures::async([]() { return; });
                wait_for_all(f1, f2, f3);
                REQUIRE(f1.is_ready());
                REQUIRE(f2.is_ready());
                REQUIRE(f3.is_ready());
            }
        }

        SECTION("Std future") {
            SECTION("Ranges") {
                std::vector<std::future<int>> fs;
                fs.emplace_back(std::async([]() { return 2; }));
                fs.emplace_back(std::async([]() { return 3; }));
                fs.emplace_back(std::async([]() { return 4; }));
                SECTION("Iterators") {
                    wait_for_all(fs.begin(), fs.end());
                    REQUIRE(std::all_of(fs.begin(), fs.end(), [](auto &f) { return futures::is_ready(f); }));
                }
                SECTION("Ranges") {
                    wait_for_all(fs);
                    REQUIRE(std::all_of(fs.begin(), fs.end(), [](auto &f) { return futures::is_ready(f); }));
                }
            }

            SECTION("Tuple") {
                auto f1 = std::async([]() { return 2; });
                auto f2 = std::async([]() { return 3.3; });
                auto f3 = std::async([]() { return; });
                wait_for_all(f1, f2, f3);
                REQUIRE(futures::is_ready(f1));
                REQUIRE(futures::is_ready(f2));
                REQUIRE(futures::is_ready(f3));
            }
        }
    }

    SECTION("Wait for any") {
        constexpr auto enough_time_for_deadlock = std::chrono::milliseconds(20);
        SECTION("Basic Future") {
            SECTION("Ranges") {
                std::vector<cfuture<int>> fs;
                fs.emplace_back(futures::async([t = enough_time_for_deadlock]() {
                    std::this_thread::sleep_for(t);
                    return 2;
                }));
                fs.emplace_back(futures::async([t = enough_time_for_deadlock]() {
                    std::this_thread::sleep_for(t);
                    return 3;
                }));
                fs.emplace_back(futures::async([t = enough_time_for_deadlock]() {
                    std::this_thread::sleep_for(t);
                    return 4;
                }));
                SECTION("Iterators") {
                    auto ready_it = wait_for_any(fs.begin(), fs.end());
                    REQUIRE(ready_it != fs.end());
                    REQUIRE(ready_it->is_ready());
                    REQUIRE(std::any_of(fs.begin(), fs.end(), [](auto &f) { return f.is_ready(); }));
                    int n = ready_it->get();
                    REQUIRE(n >= 2);
                    REQUIRE(n <= 4);
                }
                SECTION("Ranges") {
                    auto ready_it = wait_for_any(fs);
                    REQUIRE(ready_it != fs.end());
                    REQUIRE(ready_it->is_ready());
                    REQUIRE(std::any_of(fs.begin(), fs.end(), [](auto &f) { return f.is_ready(); }));
                    int n = ready_it->get();
                    REQUIRE(n >= 2);
                    REQUIRE(n <= 4);
                }
            }

            SECTION("Tuple") {
                auto f1 = futures::async([t = enough_time_for_deadlock]() {
                    std::this_thread::sleep_for(t);
                    return 2;
                });
                auto f2 = futures::async([t = enough_time_for_deadlock]() {
                    std::this_thread::sleep_for(t);
                    return 3.3;
                });
                auto f3 = futures::async([t = enough_time_for_deadlock]() {
                    std::this_thread::sleep_for(t);
                    return;
                });
                std::size_t index = wait_for_any(f1, f2, f3);
                REQUIRE(index <= 2);
                REQUIRE((f1.is_ready() || f2.is_ready() || f3.is_ready()));
                switch (index) {
                case 0: {
                    int n = f1.get();
                    REQUIRE(n == 2);
                    break;
                }
                case 1: {
                    double n = f2.get();
                    REQUIRE(n > 3.0);
                    REQUIRE(n < 3.5);
                    break;
                }
                case 2: {
                    f3.get();
                    break;
                }
                default:
                    REQUIRE(false);
                }
            }
        }

        SECTION("Std Future") {
            SECTION("Ranges") {
                std::vector<std::future<int>> fs;
                fs.emplace_back(std::async([t = enough_time_for_deadlock]() {
                    std::this_thread::sleep_for(t);
                    return 2;
                }));
                fs.emplace_back(std::async([t = enough_time_for_deadlock]() {
                    std::this_thread::sleep_for(t);
                    return 3;
                }));
                fs.emplace_back(std::async([t = enough_time_for_deadlock]() {
                    std::this_thread::sleep_for(t);
                    return 4;
                }));
                SECTION("Iterators") {
                    auto ready_it = wait_for_any(fs.begin(), fs.end());
                    REQUIRE(ready_it != fs.end());
                    REQUIRE(is_ready(*ready_it));
                    REQUIRE(std::any_of(fs.begin(), fs.end(), [](auto &f) { return is_ready(f); }));
                    int n = ready_it->get();
                    REQUIRE(n >= 2);
                    REQUIRE(n <= 4);
                }
                SECTION("Ranges") {
                    auto ready_it = wait_for_any(fs);
                    REQUIRE(ready_it != fs.end());
                    REQUIRE(is_ready(*ready_it));
                    REQUIRE(std::any_of(fs.begin(), fs.end(), [](auto &f) { return is_ready(f); }));
                    int n = ready_it->get();
                    REQUIRE(n >= 2);
                    REQUIRE(n <= 4);
                }
            }

            SECTION("Tuple") {
                auto f1 = std::async([t = enough_time_for_deadlock]() {
                    std::this_thread::sleep_for(t);
                    return 2;
                });
                auto f2 = std::async([t = enough_time_for_deadlock]() {
                    std::this_thread::sleep_for(t);
                    return 3.3;
                });
                auto f3 = std::async([t = enough_time_for_deadlock]() {
                    std::this_thread::sleep_for(t);
                    return;
                });
                std::size_t index = wait_for_any(f1, f2, f3);
                REQUIRE(index <= 2);
                REQUIRE((is_ready(f1) || is_ready(f2) || is_ready(f3)));
                switch (index) {
                case 0: {
                    int n = f1.get();
                    REQUIRE(n == 2);
                    break;
                }
                case 1: {
                    double n = f2.get();
                    REQUIRE(n > 3.0);
                    REQUIRE(n < 3.5);
                    break;
                }
                case 2: {
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
