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

    SECTION("Cancellable future") {
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

    SECTION("Continuation Stop - Shared stop source") {
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

    SECTION("Continuation Stop - Independent stop source") {
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
        f1_ss.request_stop(); // <- internal future was moved but stop source
                              // remains valid (and can be copied)
        f2.wait();            // <- wait for f1 result to arrive at f2
        REQUIRE(is_ready(f2));
        int final_count = f2.get();
        REQUIRE(final_count >= 10);
    }

    SECTION("Futures types") {
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

    SECTION("Shared futures") {
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
}