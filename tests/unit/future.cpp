#include <futures/future.hpp>
//
#include <futures/is_ready.hpp>
#include <futures/launch.hpp>
#include <futures/wait_for_all.hpp>
#include <futures/wait_for_any.hpp>
#include <futures/adaptor/then.hpp>
#include <catch2/catch.hpp>
#include <array>
#include <string>

TEST_CASE("future") {
    using namespace futures;
}

TEST_CASE("Cancellable future") {
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
        auto fn = [](stop_token const &s, std::chrono::milliseconds x) {
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

TEST_CASE("Continuation Stop - Shared stop source") {
    using namespace futures;
    using namespace std::literals;
    jcfuture<int> f1 = async(
        [&](stop_token const &st, int count) {
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

TEST_CASE("Continuation Stop - Independent stop source") {
    using namespace futures;
    using namespace std::literals;
    jcfuture<int> f1 = async(
        [](stop_token const &st, int count) {
        while (!st.stop_requested()) {
            std::this_thread::sleep_for(1ms);
            ++count;
        }
        return count;
        },
        10);
    stop_source f1_ss = f1.get_stop_source();

    jcfuture<int> f2 = then(f1, [](stop_token const &st, int count) {
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
    f1_ss.request_stop();        // <- internal future was moved but stop source
                                 // remains valid (and can be copied)
    f2.wait();                   // <- wait for f1 result to arrive at f2
    REQUIRE(is_ready(f2));
    int final_count = f2.get();
    REQUIRE(final_count >= 10);
}

TEST_CASE("Futures types") {
    using namespace futures;

    constexpr int32_t thread_pool_replicates = 100;
    SECTION("Continuable") {
        for (int32_t i = 0; i < thread_pool_replicates; ++i) {
            auto fn = [] {
                return 2;
            };
            using Function = decltype(fn);
            STATIC_REQUIRE(
                !std::is_invocable_v<std::decay_t<Function>, stop_token>);
            STATIC_REQUIRE(
                std::is_same_v<detail::launch_result_t<decltype(fn)>, int>);
            auto r = async(fn);
            REQUIRE(r.valid());
            REQUIRE(r.get() == 2);
            REQUIRE_FALSE(r.valid());
        }
    }

    SECTION("Shared") {
        STATIC_REQUIRE(!std::is_copy_constructible_v<cfuture<int>>);
        STATIC_REQUIRE(std::is_copy_constructible_v<shared_cfuture<int>>);

        using future_options_t = future_options<continuable_opt>;
        using shared_state_options = detail::
            remove_future_option_t<shared_opt, future_options_t>;
        STATIC_REQUIRE(
            std::is_same_v<
                shared_state_options,
                future_options<continuable_opt>>);

        using shared_future_options_t
            = future_options<continuable_opt, shared_opt>;
        using shared_shared_state_options = detail::
            remove_future_option_t<shared_opt, shared_future_options_t>;
        STATIC_REQUIRE(
            std::is_same_v<
                shared_shared_state_options,
                future_options<continuable_opt>>);

        auto fn = [] {
            return 2;
        };
        STATIC_REQUIRE(
            std::is_same_v<detail::launch_result_t<decltype(fn)>, int>);
        for (int32_t i = 0; i < thread_pool_replicates; ++i) {
            cfuture<int32_t> r = async(fn);
            REQUIRE(r.valid());
            shared_cfuture<int32_t> r2 = r.share();
            REQUIRE_FALSE(r.valid());
            REQUIRE(r2.valid());
            REQUIRE(r2.get() == 2);
            REQUIRE(r2.valid());
        }
    }

    SECTION("Promise / event future") {
        for (int32_t i = 0; i < thread_pool_replicates; ++i) {
            promise<int32_t> p;
            cfuture<int32_t> r = p.get_future();
            REQUIRE_FALSE(is_ready(r));
            p.set_value(2);
            REQUIRE(is_ready(r));
            REQUIRE(r.get() == 2);
        }
    }
}

TEST_CASE("Shared futures") {
    using namespace futures;
    SECTION("Basic") {
        shared_cfuture<int> f1 = async([] { return 1; }).share();
        shared_cfuture<int> f2 = f1;
        REQUIRE(f1.get() == 1);
        REQUIRE(f2.get() == 1);
        // Get twice
        REQUIRE(f1.get() == 1);
        REQUIRE(f2.get() == 1);
    }
    SECTION("Shared stop token") {
        shared_jcfuture<int> f1 = async([](stop_token const &st) {
                                      int i = 0;
                                      while (!st.stop_requested()) {
                                          ++i;
                                      }
                                      return i;
                                  }).share();
        shared_jcfuture<int> f2 = f1;
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        f2.request_stop();
        REQUIRE(f1.get() > 0);
        REQUIRE(f2.get() > 0);
        // Get twice
        REQUIRE(f1.get() > 0);
        REQUIRE(f2.get() > 0);
    }
    SECTION("Shared continuation") {
        shared_jcfuture<int> f1 = async([](stop_token const &st) {
                                      int i = 0;
                                      while (!st.stop_requested()) {
                                          ++i;
                                      }
                                      return i;
                                  }).share();
        shared_jcfuture<int> f2 = f1;
        // f3 has no stop token because f2 is shared so another task might
        // depend on it
        cfuture<int> f3 = then(f2, [](int i) {
            return i == 0 ? 0 : (1 + i % 2);
        });
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        REQUIRE_FALSE(is_ready(f1));
        REQUIRE_FALSE(is_ready(f2));
        REQUIRE_FALSE(is_ready(f3));
        f2.request_stop();
        shared_cfuture<int> f4 = f3.share();
        REQUIRE(f4.get() != 0);
        REQUIRE((f4.get() == 1 || f4.get() == 2));
    }
}

TEST_CASE("Wait") {
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
                        fs.emplace_back(
                            async_fn(std::function<int()>([]() { return 2; })));
                        fs.emplace_back(
                            async_fn(std::function<int()>([]() { return 3; })));
                        fs.emplace_back(
                            async_fn(std::function<int()>([]() { return 4; })));
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
                            FAIL("Impossible switch case");
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
