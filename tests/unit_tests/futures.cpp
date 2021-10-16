#include <array>
#include <string>

#include <catch2/catch.hpp>

#include <futures/futures.h>

TEST_CASE("Asio default executors") {
    using namespace futures;

    SECTION("Wait and stop") {
        asio::thread_pool pool(1);
        asio::thread_pool::executor_type ex = pool.executor();
        asio::thread_pool::executor_type ex2 = pool.executor();
        REQUIRE(&pool == &ex.context());
        REQUIRE(ex == ex2);
        int i = 0;
        asio::post(ex, [&]() { ++i; });
        asio::post(ex2, [&]() { ++i; });
        pool.wait(); // <- this will stop the pool
        REQUIRE(i == 2);
        asio::post(ex, [&]() { ++i; });
        pool.wait();
        REQUIRE(i == 2); // <- pool had already stopped
    }

    constexpr int thread_pool_replicates = 100;

    SECTION("Default thread pool") {
        asio::thread_pool &pool = default_execution_context();
        asio::thread_pool::executor_type ex = pool.executor();
        for (int i = 0; i < thread_pool_replicates; ++i) {
            auto f = asio::post(ex, asio::use_future([&i]() { return i * 2; }));
            REQUIRE(await(f) == i * 2);
        }
    }

    SECTION("Default executor") {
        asio::thread_pool::executor_type ex = make_default_executor();
        for (int i = 0; i < thread_pool_replicates; ++i) {
            auto f = asio::post(ex, asio::use_future([&i]() { return i * 3; }));
            REQUIRE(f.get() == i * 3);
        }
    }

    SECTION("Function precedence") {
        SECTION("Dispatch") {
            asio::thread_pool::executor_type ex = make_default_executor();
            for (int i = 0; i < thread_pool_replicates; ++i) {
                bool a = false;
                bool b = false;
                std::future<void> f1;
                std::future<void> f2;
                asio::post(ex, asio::use_future([&] {
                               f1 = asio::dispatch(ex, asio::use_future([&] { a = true; }));
                               f2 = asio::dispatch(ex, asio::use_future([&] { b = true; }));
                               REQUIRE(a);
                               REQUIRE(b);
                           }))
                    .wait();
                f1.wait();
                f2.wait();
            }
        }

        SECTION("Defer") {
            asio::thread_pool::executor_type ex = make_default_executor();
            for (int i = 0; i < thread_pool_replicates; ++i) {
                bool a = false;
                bool b = false;
                std::future<void> f1;
                std::future<void> f2;
                asio::post(ex, asio::use_future([&] {
                               f1 = asio::defer(ex, asio::use_future([&] { a = true; }));
                               f2 = asio::defer(ex, asio::use_future([&] { b = true; }));
                               REQUIRE_FALSE(a);
                               REQUIRE_FALSE(b);
                           }))
                    .wait();
                f1.wait();
                f2.wait();
            }
        }
    }
}

TEST_CASE("Future types") {
    using namespace futures;

    constexpr int thread_pool_replicates = 100;
    SECTION("Continuable") {
        for (int i = 0; i < thread_pool_replicates; ++i) {
            auto fn = [] { return 2; };
            using Function = decltype(fn);
            STATIC_REQUIRE(not std::is_invocable_v<std::decay_t<Function>, stop_token>);
            STATIC_REQUIRE(std::is_same_v<detail::async_future_result_of<Function>, cfuture<int>>);
            cfuture<int> r = async([] { return 2; });
            REQUIRE(r.valid());
            REQUIRE(r.get() == 2);
            REQUIRE_FALSE(r.valid());
        }
    }

    SECTION("Shared") {
        for (int i = 0; i < thread_pool_replicates; ++i) {
            cfuture<int> r = async([] { return 2; });
            REQUIRE(r.valid());
            shared_cfuture<int> r2 = r.share();
            REQUIRE_FALSE(r.valid());
            REQUIRE(r2.valid());
            REQUIRE(r2.get() == 2);
            REQUIRE(r2.valid());
        }
    }

    SECTION("Dispatch immediately") {
        for (int i = 0; i < thread_pool_replicates; ++i) {
            cfuture<int> r = async(launch::executor_now, [] { return 2; });
            REQUIRE(r.valid());
            REQUIRE(r.get() == 2);
            REQUIRE_FALSE(r.valid());
        }
    }

    SECTION("Promise / event future") {
        for (int i = 0; i < thread_pool_replicates; ++i) {
            std::promise<int> p;
            future<int> r = p.get_future();
            REQUIRE_FALSE(is_ready(r));
            p.set_value(2);
            REQUIRE(is_ready(r));
            REQUIRE(r.get() == 2);
        }
    }
}

TEST_CASE("Make ready") {
    using namespace futures;
    SECTION("Value") {
        auto f = make_ready_future(3);
        REQUIRE(is_ready(f));
        REQUIRE(f.get() == 3);
    }

    SECTION("Reference") {
        int a = 3;
        auto f = make_ready_future(std::reference_wrapper(a));
        REQUIRE(is_ready(f));
        REQUIRE(f.get() == 3);
    }

    SECTION("Void") {
        auto f = make_ready_future();
        REQUIRE(is_ready(f));
        REQUIRE_NOTHROW(f.get());
    }

    SECTION("Exception ptr") {
        auto f = make_exceptional_future<int>(std::make_exception_ptr(std::logic_error("error")));
        REQUIRE(is_ready(f));
        REQUIRE_THROWS(f.get());
    }

    SECTION("Exception ptr") {
        auto f = make_exceptional_future<int>(std::logic_error("error"));
        REQUIRE(is_ready(f));
        REQUIRE_THROWS(f.get());
    }
}

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

TEST_CASE("Continuation") {
    using namespace futures;
    SECTION("Default executor") {
        SECTION("Integer continuation") {
            auto c = [] { return 2; };
            auto cont = [](int v) { return v * 2; };
            cfuture<int> before = async(c);
            using Future = decltype(before);
            using Function = decltype(cont);
            STATIC_REQUIRE(std::is_same_v<detail::then_result_of_t<Function, Future>, cfuture<int>>);
            REQUIRE(before.valid());
            cfuture<int> after = then(before, [](int v) { return v * 2; });
            REQUIRE(after.get() == 4);
            REQUIRE_FALSE(before.valid());
        }

        SECTION("Continuation to void") {
            int i = 0;
            cfuture<void> before = async([&] { ++i; });
            cfuture<int> after = then(before, []() { return 2; });
            REQUIRE(after.get() == 2);
            REQUIRE(i == 1);
            REQUIRE_FALSE(before.valid());
        }

        SECTION("Void continuation") {
            int i = 0;
            cfuture<void> before = async([&] { ++i; });
            cfuture<void> after = then(before, [&]() { ++i; });
            REQUIRE_NOTHROW(after.get());
            REQUIRE(i == 2);
            REQUIRE_FALSE(before.valid());
        }
    }

    SECTION("Custom executor") {
        SECTION("First parameter") {
            SECTION("Integer continuation") {
                asio::thread_pool pool(1);
                asio::thread_pool::executor_type ex = pool.executor();
                cfuture<int> before = async([] { return 2; });
                REQUIRE(before.valid());
                cfuture<int> after = then(ex, before, [](int v) { return v * 2; });
                REQUIRE(after.get() == 4);
                REQUIRE_FALSE(before.valid());
            }

            SECTION("Continuation to void") {
                asio::thread_pool pool(1);
                asio::thread_pool::executor_type ex = pool.executor();
                int i = 0;
                cfuture<void> before = async([&] { ++i; });
                cfuture<int> after = then(ex, before, []() { return 2; });
                REQUIRE(after.get() == 2);
                REQUIRE(i == 1);
                REQUIRE_FALSE(before.valid());
            }

            SECTION("Void continuation") {
                asio::thread_pool pool(1);
                asio::thread_pool::executor_type ex = pool.executor();
                int i = 0;
                cfuture<void> before = async([&] { ++i; });
                cfuture<void> after = then(ex, before, [&]() { ++i; });
                REQUIRE_NOTHROW(after.get());
                REQUIRE(i == 2);
                REQUIRE_FALSE(before.valid());
            }
        }

        SECTION("Second parameter") {
            SECTION("Integer continuation") {
                asio::thread_pool pool(1);
                asio::thread_pool::executor_type ex = pool.executor();
                cfuture<int> before = async([] { return 2; });
                REQUIRE(before.valid());
                cfuture<int> after = then(before, ex, [](int v) { return v * 2; });
                REQUIRE(after.get() == 4);
                REQUIRE_FALSE(before.valid());
            }

            SECTION("Continuation to void") {
                asio::thread_pool pool(1);
                asio::thread_pool::executor_type ex = pool.executor();
                int i = 0;
                cfuture<void> before = async([&] { ++i; });
                cfuture<int> after = then(before, ex, []() { return 2; });
                REQUIRE(after.get() == 2);
                REQUIRE(i == 1);
                REQUIRE_FALSE(before.valid());
            }

            SECTION("Void continuation") {
                asio::thread_pool pool(1);
                asio::thread_pool::executor_type ex = pool.executor();
                int i = 0;
                cfuture<void> before = async([&] { ++i; });
                cfuture<void> after = then(before, ex, [&]() { ++i; });
                REQUIRE_NOTHROW(after.get());
                REQUIRE(i == 2);
                REQUIRE_FALSE(before.valid());
            }
        }
    }

    SECTION("Shared before") {
        SECTION("Integer continuation") {
            shared_cfuture<int> before = async([] { return 2; }).share();
            REQUIRE(before.valid());
            cfuture<int> after = then(before, [](int v) { return v * 2; });
            REQUIRE(after.get() == 4);
            REQUIRE(before.valid());
        }

        SECTION("Continuation to void") {
            int i = 0;
            shared_cfuture<void> before = async([&] { ++i; }).share();
            cfuture<int> after = then(before, []() { return 2; });
            REQUIRE(after.get() == 2);
            REQUIRE(i == 1);
            REQUIRE(before.valid());
        }

        SECTION("Void continuation") {
            int i = 0;
            shared_cfuture<void> before = async([&] { ++i; }).share();
            cfuture<void> after = then(before, [&]() { ++i; });
            REQUIRE_NOTHROW(after.get());
            REQUIRE(i == 2);
            REQUIRE(before.valid());
        }
    }

    SECTION("Operator>>") {
        SECTION("Separate objects") {
            SECTION("Integer continuation") {
                cfuture<int> before = async([] { return 2; });
                REQUIRE(before.valid());
                cfuture<int> after = before >> [](int v) { return v * 2; };
                REQUIRE(after.get() == 4);
                REQUIRE_FALSE(before.valid());
            }

            SECTION("Continuation to void") {
                int i = 0;
                cfuture<void> before = async([&] { ++i; });
                cfuture<int> after = before >> []() { return 2; };
                REQUIRE(after.get() == 2);
                REQUIRE(i == 1);
                REQUIRE_FALSE(before.valid());
            }

            SECTION("Void continuation") {
                int i = 0;
                cfuture<void> before = async([&] { ++i; });
                cfuture<void> after = before >> [&]() { ++i; };
                REQUIRE_NOTHROW(after.get());
                REQUIRE(i == 2);
                REQUIRE_FALSE(before.valid());
            }
        }

        SECTION("Chaining tasks") {
            SECTION("Integer continuation") {
                cfuture<int> after = async([] { return 2; }) >> [](int v) { return v * 2; };
                REQUIRE(after.get() == 4);
            }

            SECTION("Continuation to void") {
                int i = 0;
                cfuture<int> after = async([&] { ++i; }) >> []() { return 2; };
                REQUIRE(after.get() == 2);
                REQUIRE(i == 1);
            }

            SECTION("Void continuation") {
                int i = 0;
                cfuture<void> after = async([&] { ++i; }) >> [&]() { ++i; };
                REQUIRE_NOTHROW(after.get());
                REQUIRE(i == 2);
            }
        }

        SECTION("Custom executor - Separate objects") {
            SECTION("Integer continuation") {
                auto ex = make_inline_executor();
                cfuture<int> before = async([] { return 2; });
                REQUIRE(before.valid());
                cfuture<int> after = before >> ex % [](int v) { return v * 2; };
                REQUIRE(after.get() == 4);
                REQUIRE_FALSE(before.valid());
            }

            SECTION("Continuation to void") {
                auto ex = make_inline_executor();
                int i = 0;
                cfuture<void> before = async([&] { ++i; });
                cfuture<int> after = before >> ex % []() { return 2; };
                REQUIRE(after.get() == 2);
                REQUIRE(i == 1);
                REQUIRE_FALSE(before.valid());
            }

            SECTION("Void continuation") {
                auto ex = make_inline_executor();
                int i = 0;
                cfuture<void> before = async([&] { ++i; });
                cfuture<void> after = before >> ex % [&]() { ++i; };
                REQUIRE_NOTHROW(after.get());
                REQUIRE(i == 2);
                REQUIRE_FALSE(before.valid());
            }
        }

        SECTION("Custom executor - Chaining tasks") {
            SECTION("Integer continuation") {
                auto ex = make_inline_executor();
                cfuture<int> after = async([] { return 2; }) >> ex % [](int v) { return v * 2; };
                REQUIRE(after.get() == 4);
            }

            SECTION("Continuation to void") {
                auto ex = make_inline_executor();
                int i = 0;
                cfuture<int> after = async([&] { ++i; }) >> ex % []() { return 2; };
                REQUIRE(after.get() == 2);
                REQUIRE(i == 1);
            }

            SECTION("Void continuation") {
                auto ex = make_inline_executor();
                int i = 0;
                cfuture<void> after = async([&] { ++i; }) >> ex % [&]() { ++i; };
                REQUIRE_NOTHROW(after.get());
                REQUIRE(i == 2);
            }
        }
    }

    SECTION("Future unwrapping for continuations") {
        SECTION("No args for void") {
            int i = 0;
            cfuture<void> f1 = async([&i]() { ++i; });
            cfuture<int> f2 = f1 >> []() { return 6; };
            REQUIRE(f2.get() == 6);
            REQUIRE(i == 1);
        }

        SECTION("Nothing to unwrap") {
            cfuture<int> f1 = async([]() { return 3; });
            cfuture<int> f2 = f1 >> [](int a) { return a * 2; };
            REQUIRE(f2.get() == 6);
        }

        SECTION("Future future unwrap") {
            cfuture<future<int>> f1 = async([]() {
                future<int> f1a = make_ready_future(3);
                return f1a;
            });
            cfuture<int> f2 = f1 >> [](int a) { return a * 2; };
            REQUIRE(f2.get() == 6);
        }

        SECTION("Tuple unwrap") {
            cfuture<std::tuple<int, int, int>> f1 = async([]() { return std::make_tuple(1, 2, 3); });
            cfuture<int> f2 = f1 >> [](int a, int b, int c) { return a * b * c; };
            REQUIRE(f2.get() == 6);
        }

        SECTION("Tuple of futures unwrap") {
            cfuture<std::tuple<future<int>, future<int>, future<int>>> f1 = async(
                []() { return std::make_tuple(make_ready_future(1), make_ready_future(2), make_ready_future(3)); });
            cfuture<int> f2 = f1 >> [](int a, int b, int c) { return a * b * c; };
            REQUIRE(f2.get() == 6);
        }
    }
}

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
                auto continuation = [](small::vector<cfuture<int>> rs) {
                    return rs[0].get() + rs[1].get() + rs[2].get();
                };
                STATIC_REQUIRE(is_future_v<decltype(f)>);
                STATIC_REQUIRE(is_future_continuation_v<decltype(continuation), decltype(f)>);
                auto f4 = then(f, continuation);
                REQUIRE(f4.get() == 2 + 3 + 4);
            }

            SECTION("Continue with lvalue") {
                auto continuation = [](small::vector<cfuture<int>> &rs) {
                    return rs[0].get() + rs[1].get() + rs[2].get();
                };
                STATIC_REQUIRE(is_future_v<decltype(f)>);
                STATIC_REQUIRE(not is_future_v<decltype(continuation)>);
                STATIC_REQUIRE(is_future_continuation_v<decltype(continuation), decltype(f)>);
                auto f4 = then(f, continuation);
                REQUIRE(f4.get() == 2 + 3 + 4);
            }

            SECTION("Continue with const lvalue") {
                auto continuation = [](const small::vector<cfuture<int>> &) {
                    return 2 + 3 + 4; // <- cannot get from const future :P
                };
                STATIC_REQUIRE(is_future_v<decltype(f)>);
                STATIC_REQUIRE(not is_future_v<decltype(continuation)>);
                STATIC_REQUIRE(is_future_continuation_v<decltype(continuation), decltype(f)>);
                auto f4 = then(f, continuation);
                REQUIRE(f4.get() == 2 + 3 + 4);
            }

            SECTION("Continue with rvalue") {
                auto continuation = [](small::vector<cfuture<int>> &&rs) {
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
                auto continuation = [](small::vector<int> rs) { return rs[0] + rs[1] + rs[2]; };
                STATIC_REQUIRE(is_future_v<decltype(f)>);
                STATIC_REQUIRE(is_future_continuation_v<decltype(continuation), decltype(f)>);
                auto f4 = then(f, continuation);
                REQUIRE(f4.get() == 2 + 3 + 4);
            }

            SECTION("Continue with lvalue") {
                auto continuation = [](small::vector<int> &rs) { return rs[0] + rs[1] + rs[2]; };
                STATIC_REQUIRE(is_future_v<decltype(f)>);
                STATIC_REQUIRE(is_future_continuation_v<decltype(continuation), decltype(f)>);
                auto f4 = then(f, continuation);
                REQUIRE(f4.get() == 2 + 3 + 4);
            }

            SECTION("Continue with const lvalue") {
                auto continuation = [](const small::vector<int> &rs) { return rs[0] + rs[1] + rs[2]; };
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

TEST_CASE("Disjunction") {
    using namespace futures;
    SECTION("Empty disjunction") {
        auto f = when_any();
        REQUIRE(f.valid());
        REQUIRE_NOTHROW(f.wait());
        REQUIRE(f.wait_for(std::chrono::seconds(0)) == std::future_status::ready);
        REQUIRE(f.wait_until(std::chrono::system_clock::now() + std::chrono::seconds(0)) == std::future_status::ready);
        REQUIRE(is_ready(f));
        when_any_result r = f.get();
        REQUIRE(r.index == size_t(-1));
        REQUIRE(r.tasks == std::make_tuple());
    }

    SECTION("Single disjunction") {
        auto f1 = async([]() { return 2; });
        auto f = when_any(f1);
        REQUIRE(f.valid());
        f.wait();
        REQUIRE(f.wait_for(std::chrono::seconds(0)) == std::future_status::ready);
        REQUIRE(f.wait_until(std::chrono::system_clock::now() + std::chrono::seconds(0)) == std::future_status::ready);
        REQUIRE(is_ready(f));
        when_any_result r = f.get();
        REQUIRE(r.index == size_t(0));
        REQUIRE(std::get<0>(r.tasks).get() == 2);
    }

    SECTION("Tuple disjunction") {
        cfuture<int> f1 = async([]() { return 2; });
        cfuture<double> f2 = async([]() { return 3.5; });
        cfuture<std::string> f3 = async([]() -> std::string { return "name"; });
        when_any_future<std::tuple<cfuture<int>, cfuture<double>, cfuture<std::string>>> f = when_any(f1, f2, f3);
        REQUIRE(f.valid());
        REQUIRE_FALSE(f1.valid());
        REQUIRE_FALSE(f2.valid());
        REQUIRE_FALSE(f3.valid());

        SECTION("Wait") {
            f.wait();
            std::future_status s1 = f.wait_for(std::chrono::seconds(0));
            REQUIRE(s1 == std::future_status::ready);
            std::future_status s2 = f.wait_until(std::chrono::system_clock::now() + std::chrono::seconds(0));
            REQUIRE(s2 == std::future_status::ready);
            REQUIRE(is_ready(f));
            when_any_result any_r = f.get();
            size_t i = any_r.index;
            auto [r1, r2, r3] = std::move(any_r.tasks);
            REQUIRE(i < 3);
            if (i == 0) {
                REQUIRE(r1.get() == 2);
            } else if (i == 1) {
                REQUIRE(r2.get() > 3.0);
            } else {
                REQUIRE(r3.get() == "name");
            }
        }

        SECTION("Continue") {
            auto continuation = [](when_any_result<std::tuple<cfuture<int>, cfuture<double>, cfuture<std::string>>> r) {
                if (r.index == 0) {
                    return std::get<0>(r.tasks).get();
                } else if (r.index == 1) {
                    return static_cast<int>(std::get<1>(r.tasks).get());
                } else if (r.index == 2) {
                    return static_cast<int>(std::get<2>(r.tasks).get().size());
                }
                return 0;
            };
            auto f4 = then(f, continuation);
            int r = f4.get();
            REQUIRE_FALSE(r == 0);
            REQUIRE((r == 2 || r == 3 || r == 4));
        }

        SECTION("Unwrap to tuple of futures") {
            auto continuation = [](size_t index,
                                   std::tuple<cfuture<int>, cfuture<double>, cfuture<std::string>> tasks) {
                if (index == 0) {
                    return std::get<0>(tasks).get();
                } else if (index == 1) {
                    return static_cast<int>(std::get<1>(tasks).get());
                } else if (index == 2) {
                    return static_cast<int>(std::get<2>(tasks).get().size());
                }
                return 0;
            };
            using tuple_type = std::tuple<cfuture<int>, cfuture<double>, cfuture<std::string>>;
            using result_type = when_any_result<tuple_type>;
            STATIC_REQUIRE(detail::is_when_any_result_v<result_type>);
            STATIC_REQUIRE(detail::is_tuple_when_any_result_v<result_type>);
            STATIC_REQUIRE(
                detail::is_index_and_sequence_invocable<decltype(continuation), std::tuple<>, result_type>::value);
            auto f4 = then(f, continuation);
            int r = f4.get();
            REQUIRE_FALSE(r == 0);
            REQUIRE((r == 2 || r == 3 || r == 4));
        }

        SECTION("Unwrap to futures") {
            auto continuation = [](size_t index, cfuture<int> f1, cfuture<double> f2, cfuture<std::string> f3) {
                if (index == 0) {
                    return f1.get();
                } else if (index == 1) {
                    return static_cast<int>(f2.get());
                } else if (index == 2) {
                    return static_cast<int>(f3.get().size());
                }
                return 0;
            };
            auto f4 = then(f, continuation);
            int r = f4.get();
            REQUIRE_FALSE(r == 0);
            REQUIRE((r == 2 || r == 3 || r == 4));
        }
    }

    SECTION("Disjuction unwrap to value") {
        // We can unwrap disjuntion results to a single value only when all futures have the same type
        auto f1 = async([]() { return 2; });
        auto f2 = async([]() { return 3; });
        auto f3 = async([]() { return 4; });
        auto f = when_any(f1, f2, f3);
        REQUIRE(f.valid());
        REQUIRE_FALSE(f1.valid());
        REQUIRE_FALSE(f2.valid());
        REQUIRE_FALSE(f3.valid());

        SECTION("Unwrap to common future type") {
            // We can unwrap that here because all futures return int
            auto continuation = [](cfuture<int> r) { return r.get() * 3; };
            STATIC_REQUIRE(is_future_v<decltype(f)>);
            STATIC_REQUIRE(is_future_continuation_v<decltype(continuation), decltype(f)>);
            auto f4 = then(f, continuation);
            int r = f4.get();
            REQUIRE((r == 6 || r == 9 || r == 12));
        }

        SECTION("Unwrap to common value type") {
            // We can unwrap that here because all futures return int
            auto continuation = [](int r) { return r * 3; };
            STATIC_REQUIRE(is_future_v<decltype(f)>);
            STATIC_REQUIRE(is_future_continuation_v<decltype(continuation), decltype(f)>);
            auto f4 = then(f, continuation);
            int r = f4.get();
            REQUIRE((r == 6 || r == 9 || r == 12));
        }
    }

    SECTION("Tuple disjunction with lambdas") {
        auto f1 = async([]() { return 2; });
        auto f2 = []() { return 3.5; };
        REQUIRE(f1.valid());
        auto f = when_any(f1, f2);
        REQUIRE(f.valid());
        REQUIRE_FALSE(f1.valid());
        auto any = f.get();
        if (any.index == 0) {
            int i = std::get<0>(any.tasks).get();
            REQUIRE(i == 2);
        } else {
            double d = std::get<1>(any.tasks).get();
            REQUIRE(d > 3.);
            REQUIRE(d < 4.);
        }
    }

    SECTION("Range disjunction") {
        std::vector<cfuture<int>> range;
        range.emplace_back(async([]() { return 2; }));
        range.emplace_back(async([]() { return 3; }));
        range.emplace_back(async([]() { return 4; }));
        auto f = when_any(range);
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
            if (rs.index == 0) {
                REQUIRE(rs.tasks[0].get() == 2);
            } else if (rs.index == 1) {
                REQUIRE(rs.tasks[1].get() == 3);
            } else if (rs.index == 2) {
                REQUIRE(rs.tasks[2].get() == 4);
            }
        }

        SECTION("Continue") {
            auto continuation = [](when_any_result<small::vector<cfuture<int>>> r) {
                if (r.index == 0) {
                    return r.tasks[0].get();
                } else if (r.index == 1) {
                    return r.tasks[1].get();
                } else if (r.index == 2) {
                    return r.tasks[2].get();
                }
                return 0;
            };
            STATIC_REQUIRE(is_future_v<decltype(f)>);
            STATIC_REQUIRE(is_future_continuation_v<decltype(continuation), decltype(f)>);
            auto f4 = then(f, continuation);
            int r = f4.get();
            REQUIRE_FALSE(r == 0);
            REQUIRE((r == 2 || r == 3 || r == 4));
        }

        SECTION("Unwrap to tuple of futures") {
            auto continuation = [](size_t index, small::vector<cfuture<int>> tasks) {
                if (index == 0) {
                    return tasks[0].get();
                } else if (index == 1) {
                    return tasks[1].get();
                } else if (index == 2) {
                    return tasks[2].get();
                }
                return 0;
            };
            using tuple_type = small::vector<cfuture<int>>;
            using result_type = when_any_result<tuple_type>;
            STATIC_REQUIRE(detail::is_when_any_result_v<result_type>);
            STATIC_REQUIRE(
                detail::is_index_and_sequence_invocable<decltype(continuation), std::tuple<>, result_type>::value);
            auto f4 = then(f, continuation);
            int r = f4.get();
            REQUIRE_FALSE(r == 0);
            REQUIRE((r == 2 || r == 3 || r == 4));
        }

        SECTION("Unwrap to common future type") {
            // We can unwrap that here because all vector futures return int
            auto continuation = [](cfuture<int> r) { return r.get() * 3; };
            auto f4 = then(f, continuation);
            int r = f4.get();
            REQUIRE((r == 6 || r == 9 || r == 12));
        }

        SECTION("Unwrap to common value type") {
            // We can unwrap that here because all futures return int
            auto continuation = [](int r) { return r * 3; };
            STATIC_REQUIRE(is_future_v<decltype(f)>);
            STATIC_REQUIRE(is_future_continuation_v<decltype(continuation), decltype(f)>);
            auto f4 = then(f, continuation);
            int r = f4.get();
            REQUIRE((r == 6 || r == 9 || r == 12));
        }
    }

    SECTION("Range disjunction with lambdas") {
        std::function<int()> f1 = []() { return 2; };
        std::function<int()> f2 = []() { return 3; };
        std::vector<std::function<int()>> range;
        range.emplace_back(f1);
        range.emplace_back(f2);
        auto f = when_any(range);
        REQUIRE(f.valid());
        auto rs = f.get();
        if (rs.index == 0) {
            REQUIRE(rs.tasks[0].get() == 2);
        } else if (rs.index == 1) {
            REQUIRE(rs.tasks[1].get() == 3);
        }
    }

    SECTION("Operator||") {
        SECTION("Future disjunction") {
            cfuture<int> f1 = async([] { return 1; });
            cfuture<int> f2 = async([] { return 2; });
            auto f = f1 || f2;
            REQUIRE_FALSE(f1.valid());
            REQUIRE_FALSE(f2.valid());
            auto [index, tasks] = f.get();
            if (index == 0) {
                REQUIRE(std::get<0>(tasks).get() == 1);
            } else {
                REQUIRE(std::get<1>(tasks).get() == 2);
            }
        }

        SECTION("Lambda disjunction") {
            // When a lambda is passed to when_all, it is converted into a future immediately
            auto f = [] { return 1; } || [] { return 2; };
            auto [index, tasks] = f.get();
            if (index == 0) {
                REQUIRE(std::get<0>(tasks).get() == 1);
            } else {
                REQUIRE(std::get<1>(tasks).get() == 2);
            }
        }

        SECTION("Future/lambda disjunction") {
            cfuture<int> f1 = async([] { return 1; });
            auto f = f1 || [] { return 2; };
            REQUIRE_FALSE(f1.valid());
            auto [index, tasks] = f.get();
            if (index == 0) {
                REQUIRE(std::get<0>(tasks).get() == 1);
            } else {
                REQUIRE(std::get<1>(tasks).get() == 2);
            }
        }

        SECTION("Lambda/future disjunction") {
            cfuture<int> f2 = async([] { return 2; });
            auto f = [] { return 1; } || f2;
            REQUIRE_FALSE(f2.valid());
            auto [index, tasks] = f.get();
            if (index == 0) {
                REQUIRE(std::get<0>(tasks).get() == 1);
            } else {
                REQUIRE(std::get<1>(tasks).get() == 2);
            }
        }

        SECTION("Concatenate disjunctions") {
            cfuture<int> f1 = async([] { return 1; });
            cfuture<int> f2 = async([] { return 2; });
            cfuture<int> f3 = async([] { return 3; });
            auto f = f1 || f2 || f3 || []() { return 4; };
            REQUIRE_FALSE(f1.valid());
            REQUIRE_FALSE(f2.valid());
            REQUIRE_FALSE(f3.valid());
            auto [index, tasks] = f.get();
            if (index == 0) {
                REQUIRE(std::get<0>(tasks).get() == 1);
            } else if (index == 1) {
                REQUIRE(std::get<1>(tasks).get() == 2);
            } else if (index == 2) {
                REQUIRE(std::get<2>(tasks).get() == 3);
            } else if (index == 3) {
                REQUIRE(std::get<3>(tasks).get() == 4);
            }
        }
    }

    SECTION("Disjunction continuation") {
        cfuture<int> f1 = async([] { return 1; });
        cfuture<int> f2 = async([] { return 2; });
        cfuture<int> f3 = async([] { return 3; });
        using Tuple = std::tuple<cfuture<int>, cfuture<int>, cfuture<int>, std::future<int>>;
        using Future = when_any_future<Tuple>;
        Future f = f1 || f2 || f3 || []() { return 4; };
        STATIC_REQUIRE(std::is_same_v<unwrap_future_t<Future>, when_any_result<Tuple>>);
        auto c = f >> [](int a) { return a * 5; } >> [](int s) { return s * 5; };
        int r = c.get();
        REQUIRE(r >= 25);
        REQUIRE(r <= 100);
    }
}

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
                int i = 0;
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
            REQUIRE(t > 2.2);
        }

        SECTION("Stop from original source") {
            // the stop state is not invalided in the original future
            f.request_stop();
            double t = f2.get();
            REQUIRE(t > 2.2);
        }
    }
}

TEST_CASE("Continuation stop") {
    using namespace futures;
    using namespace std::literals;
    jcfuture<int> f1 = async(
        [&](const stop_token &st, int count) {
            while (not st.stop_requested()) {
                std::this_thread::sleep_for(1ms);
                ++count;
            }
            return count;
        },
        0);

    SECTION("Shared stop source") {
        jcfuture<int> f2 = then(f1, [&](int count) { return count * 2; });
        std::this_thread::sleep_for(100ms);
        REQUIRE_FALSE(is_ready(f2));
        f2.request_stop(); // <- inherited from f1
        f2.wait();
        REQUIRE(is_ready(f2));
        int final_count = f2.get();
        REQUIRE(final_count > 0);
    }

    SECTION("Independent stop source") {
        jcfuture<int> f2 = then(f1, [&](const stop_token &st, int count) {
            while (not st.stop_requested()) {
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
        f1.request_stop(); // <- internal future was moved but stop source remains the same (and can be copied)
        f2.wait();         // <- wait for f1 result to arrive at f2
        REQUIRE(is_ready(f2));
        int final_count = f2.get();
        REQUIRE(final_count > 0);
    }
}

TEST_CASE("Exceptions") {
    using namespace futures;
    SECTION("Basic") {
        auto f1 = async([] { throw std::runtime_error("error"); });
        f1.wait();
        bool caught = false;
        try {
            f1.get();
        } catch (...) {
            caught = true;
        }
        REQUIRE(caught);
    }
    SECTION("Continuations") {
        auto f1 = async([] { throw std::runtime_error("error"); });
        auto f2 = then(f1, []() { return; });
        f2.wait();
        bool caught = false;
        try {
            f2.get();
        } catch (...) {
            caught = true;
        }
        REQUIRE(caught);
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
        shared_jcfuture<int> f1 = async([](const stop_token &st) {
                                      int i = 0;
                                      while (not st.stop_requested()) {
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
        shared_jcfuture<int> f1 = async([](const stop_token &st) {
                                      int i = 0;
                                      while (not st.stop_requested()) {
                                          ++i;
                                      }
                                      return i;
                                  }).share();
        shared_jcfuture<int> f2 = f1;
        // f3 has no stop token because f2 is shared so another task might depend on it
        cfuture<int> f3 = then(f2, [](int i) { return i == 0 ? 0 : (1 + i % 2); });
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