#include <futures/futures.hpp>
#include <futures/detail/deps/boost/mp11/algorithm.hpp>
#include <array>
#include <string>
#include <catch2/catch.hpp>

TEST_CASE(TEST_CASE_PREFIX "Continuation") {
    using namespace futures;
    SECTION("Default executor") {
        SECTION("Integer continuation") {
            auto c = [] {
                return 2;
            };
            auto cont = [](int v) {
                return v * 2;
            };
            cfuture<int> before = async(c);
            using Future = decltype(before);
            using Function = decltype(cont);

            REQUIRE(before.valid());
            auto after_fn = [](int v) {
                return v * 2;
            };

            using unwrap_result_no_token_type = detail::
                continue_invoke_result_t<Future, Function>;

            STATIC_REQUIRE(std::is_same_v<unwrap_result_no_token_type, int>);
            static constexpr bool is_valid_without_stop_token = !std::is_same_v<
                unwrap_result_no_token_type,
                detail::continue_tags::failure>;
            STATIC_REQUIRE(is_valid_without_stop_token);

            STATIC_REQUIRE(
                detail::next_future_traits<
                    default_executor_type,
                    std::decay_t<decltype(after_fn)>,
                    std::decay_t<cfuture<int>>>::is_valid);
            cfuture<int> after = then(before, after_fn);
            REQUIRE(after.get() == 4);
            REQUIRE_FALSE(before.valid());
        }

        SECTION("Continuation to void") {
            int i = 0;
            cfuture<void> before = async([&i] { ++i; });
            cfuture<int> after = then(before, []() { return 2; });
            REQUIRE(after.get() == 2);
            REQUIRE(i == 1);
            REQUIRE_FALSE(before.valid());
        }

        SECTION("Void continuation") {
            int i = 0;
            cfuture<void> before = async([&] { ++i; });
            auto cont = [&]() {
                ++i;
            };
            cfuture<void> after = then(before, cont);
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
                cfuture<int> after = then(ex, before, [](int v) {
                    return v * 2;
                });
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
                cfuture<int> after = then(ex, before, [](int v) {
                    return v * 2;
                });
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
                cfuture<void> before = async([&i] { ++i; });
                cfuture<void> after = then(ex, before, [&i]() { ++i; });
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
                cfuture<int> after = before >> [](int v) {
                    return v * 2;
                };
                REQUIRE(after.get() == 4);
                REQUIRE_FALSE(before.valid());
            }

            SECTION("Continuation to void") {
                int i = 0;
                cfuture<void> before = async([&] { ++i; });
                cfuture<int> after = before >> []() {
                    return 2;
                };
                REQUIRE(after.get() == 2);
                REQUIRE(i == 1);
                REQUIRE_FALSE(before.valid());
            }

            SECTION("Void continuation") {
                int i = 0;
                cfuture<void> before = async([&] { ++i; });
                cfuture<void> after = before >> [&]() {
                    ++i;
                };
                REQUIRE_NOTHROW(after.get());
                REQUIRE(i == 2);
                REQUIRE_FALSE(before.valid());
            }
        }

        SECTION("Chaining tasks") {
            SECTION("Integer continuation") {
                cfuture<int> after = async([] { return 2; }) >> [](int v) {
                    return v * 2;
                };
                REQUIRE(after.get() == 4);
            }

            SECTION("Continuation to void") {
                int i = 0;
                cfuture<int> after = async([&] { ++i; }) >> []() {
                    return 2;
                };
                REQUIRE(after.get() == 2);
                REQUIRE(i == 1);
            }

            SECTION("Void continuation") {
                int i = 0;
                cfuture<void> after = async([&] { ++i; }) >> [&]() {
                    ++i;
                };
                REQUIRE_NOTHROW(after.get());
                REQUIRE(i == 2);
            }
        }

        SECTION("Custom executor - Separate objects") {
            SECTION("Integer continuation") {
                auto ex = make_inline_executor();
                cfuture<int> before = async([] { return 2; });
                REQUIRE(before.valid());
                auto after = before >> ex % [](int v) {
                    return v * 2;
                };
                REQUIRE(after.get() == 4);
                REQUIRE_FALSE(before.valid());
            }

            SECTION("Continuation to void") {
                auto ex = make_inline_executor();
                int i = 0;
                cfuture<void> before = async([&] { ++i; });
                auto after = before >> ex % []() {
                    return 2;
                };
                REQUIRE(after.get() == 2);
                REQUIRE(i == 1);
                REQUIRE_FALSE(before.valid());
            }

            SECTION("Void continuation") {
                auto ex = make_inline_executor();
                int i = 0;
                cfuture<void> before = async([&] { ++i; });
                auto after = before >> ex % [&]() {
                    ++i;
                };
                REQUIRE_NOTHROW(after.get());
                REQUIRE(i == 2);
                REQUIRE_FALSE(before.valid());
            }
        }

        SECTION("Custom executor - Chaining tasks") {
            SECTION("Integer continuation") {
                auto ex = make_inline_executor();
                auto after = async([] { return 2; }) >> ex % [](int v) {
                    return v * 2;
                };
                REQUIRE(after.get() == 4);
            }

            SECTION("Continuation to void") {
                auto ex = make_inline_executor();
                int i = 0;
                auto after = async([&i] { ++i; }) >> ex % []() {
                    return 2;
                };
                REQUIRE(after.get() == 2);
                REQUIRE(i == 1);
            }

            SECTION("Void continuation") {
                auto ex = make_inline_executor();
                int i = 0;
                auto after = async([&] { ++i; }) >> ex % [&]() {
                    ++i;
                };
                REQUIRE_NOTHROW(after.get());
                REQUIRE(i == 2);
            }
        }
    }

    SECTION("Future unwrapping for continuations") {
        SECTION("No args for void") {
            int i = 0;
            cfuture<void> f1 = async([&i]() { ++i; });
            cfuture<int> f2 = f1 >> []() {
                return 6;
            };
            REQUIRE(f2.get() == 6);
            REQUIRE(i == 1);
        }

        SECTION("Nothing to unwrap") {
            cfuture<int> f1 = async([]() { return 3; });
            cfuture<int> f2 = f1 >> [](int a) {
                return a * 2;
            };
            REQUIRE(f2.get() == 6);
        }

        SECTION("Future future unwrap") {
            auto f1 = async([]() {
                auto f1a = make_ready_future(3);
                return f1a;
            });
            cfuture<int> f2 = f1 >> [](int a) {
                return a * 2;
            };
            REQUIRE(f2.get() == 6);
        }

        SECTION("Tuple unwrap") {
            cfuture<std::tuple<int, int, int>> f1 = async([]() {
                return std::make_tuple(1, 2, 3);
            });
            cfuture<int> f2 = f1 >> [](int a, int b, int c) {
                return a * b * c;
            };
            REQUIRE(f2.get() == 6);
        }

        SECTION("Tuple of futures unwrap") {
            auto f1_function = []() {
                return std::make_tuple(
                    make_ready_future(1),
                    make_ready_future(2),
                    make_ready_future(3));
            };
            auto f1 = async(f1_function);
            auto continue_fn = [](int a, int b, int c) {
                return a * b * c;
            };

            using Future = decltype(f1);
            using Function = decltype(continue_fn);

            using value_type = future_value_t<Future>;
            constexpr bool tuple_explode = boost::mp11::mp_apply<
                std::is_invocable,
                boost::mp11::mp_append<std::tuple<Function>, value_type>>::value;
            STATIC_REQUIRE(!tuple_explode);
            constexpr bool is_future_tuple = boost::mp11::
                mp_all_of<value_type, is_future>::value;
            STATIC_REQUIRE(is_future_tuple);
            using unwrapped_elements = boost::mp11::
                mp_transform<future_value_t, value_type>;
            STATIC_REQUIRE(
                std::is_same_v<unwrapped_elements, std::tuple<int, int, int>>);
            constexpr bool tuple_explode_unwrap = boost::mp11::mp_apply<
                std::is_invocable,
                boost::mp11::
                    mp_append<std::tuple<Function>, unwrapped_elements>>::value;
            STATIC_REQUIRE(tuple_explode_unwrap);

            cfuture<int> f2 = f1 >> [](int a, int b, int c) {
                return a * b * c;
            };
            REQUIRE(f2.get() == 6);
        }
    }
}
