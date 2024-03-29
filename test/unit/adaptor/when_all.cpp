#include <futures/adaptor/when_all.hpp>
//
#include <futures/is_ready.hpp>
#include <futures/adaptor/then.hpp>
#include <catch2/catch.hpp>
#include <array>
#include <string>

#if defined(__GNUG__) && !defined(__clang__)
#    pragma GCC diagnostic push
#    pragma GCC diagnostic ignored "-Wmaybe-uninitialized"
#endif

TEST_CASE("when_all") {
    using namespace futures;
    SECTION("Empty conjunction") {
        auto f = when_all();
        REQUIRE(f.valid());
        REQUIRE_NOTHROW(f.wait());
        REQUIRE(f.wait_for(std::chrono::seconds(0)) == future_status::ready);
        REQUIRE(
            f.wait_until(
                std::chrono::system_clock::now() + std::chrono::seconds(0))
            == future_status::ready);
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
            REQUIRE_NOTHROW(
                f.wait_for(std::chrono::seconds(0)) == future_status::ready);
            REQUIRE_NOTHROW(
                f.wait_until(
                    std::chrono::system_clock::now() + std::chrono::seconds(0))
                == future_status::ready);
            REQUIRE(is_ready(f));
            cfuture<int> r1;
            cfuture<double> r2;
            cfuture<std::string> r3;
            std::tie(r1, r2, r3) = f.get();
            REQUIRE(r1.get() == 2);
            double d = r2.get();
            REQUIRE(d >= 3.0);
            REQUIRE(d <= 4.0);
            REQUIRE(r3.get() == "name");
        }

        SECTION("Continue") {
            auto continuation =
                [](std::tuple<cfuture<int>, cfuture<double>, cfuture<std::string>>
                       r) {
                return std::get<0>(r).get()
                       + static_cast<int>(std::get<1>(r).get())
                       + static_cast<int>(std::get<2>(r).get().size());
            };
            STATIC_REQUIRE(is_future_like_v<decltype(f)>);
            STATIC_REQUIRE(
                detail::next_future_traits<
                    default_executor_type,
                    std::decay_t<decltype(continuation)>,
                    std::decay_t<decltype(f)>>::is_valid);
            STATIC_REQUIRE(
                std::is_same<
                    int,
                    detail::invoke_result_t<
                        decltype(continuation),
                        std::tuple<
                            cfuture<int>,
                            cfuture<double>,
                            cfuture<std::string>>>>::value);
            STATIC_REQUIRE(
                std::is_same<
                    cfuture<int>,
                    decltype(detail::internal_then(
                        ::futures::make_default_executor(),
                        f,
                        continuation))>::value);
            STATIC_REQUIRE(
                std::is_same<cfuture<int>, decltype(then(f, continuation))>::
                    value);
            auto f4 = then(f, continuation);
            REQUIRE(f4.get() == 2 + 3 + 4);
        }

        SECTION("Unwrap to futures") {
            auto f4 = then(
                f,
                [](cfuture<int> r1,
                   cfuture<double> r2,
                   cfuture<std::string> r3) {
                return r1.get() + static_cast<int>(r2.get()) + r3.get().size();
                });
            REQUIRE(f4.get() == 2 + 3 + 4);
        }

        SECTION("Unwrap to values") {
            auto f4 = then(f, [](int r1, double r2, std::string const &r3) {
                return r1 + static_cast<int>(r2) + r3.size();
            });
            REQUIRE(f4.get() == 2 + 3 + 4);
        }
    }

    SECTION("Tuple conjunction with lambdas") {
        auto f1 = async([]() { return 2; });
        auto f2 = []() {
            return 3.5;
        };
        REQUIRE(f1.valid());
        auto f = when_all(f1, f2);
        REQUIRE(f.valid());
        REQUIRE_FALSE(f1.valid());
        cfuture<int> r1;
        cfuture<double> r2;
        std::tie(r1, r2) = f.get();
        REQUIRE(r1.get() == 2);
        double d = r2.get();
        REQUIRE(d > 3.);
        REQUIRE(d < 4.);
    }

    SECTION("Range conjunction") {
        detail::small_vector<cfuture<int>> range;
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
            REQUIRE_NOTHROW(
                f.wait_for(std::chrono::seconds(0)) == future_status::ready);
            REQUIRE_NOTHROW(
                f.wait_until(
                    std::chrono::system_clock::now() + std::chrono::seconds(0))
                == future_status::ready);
            REQUIRE(is_ready(f));
            auto rs = f.get();
            REQUIRE(rs[0].get() == 2);
            REQUIRE(rs[1].get() == 3);
            REQUIRE(rs[2].get() == 4);
        }

        SECTION("No unwrapping") {
            SECTION("Continue with value") {
                auto continuation = [](detail::small_vector<cfuture<int>> rs) {
                    return rs[0].get() + rs[1].get() + rs[2].get();
                };
                STATIC_REQUIRE(is_future_like_v<decltype(f)>);
                STATIC_REQUIRE(
                    detail::next_future_traits<
                        default_executor_type,
                        std::decay_t<decltype(continuation)>,
                        std::decay_t<decltype(f)>>::is_valid);
                auto f4 = then(f, continuation); // continue from when_all
                REQUIRE(f4.get() == 2 + 3 + 4);
            }

            SECTION("Continue with lvalue") {
                auto continuation = [](detail::small_vector<cfuture<int>> rs) {
                    return rs[0].get() + rs[1].get() + rs[2].get();
                };
                using Future = decltype(f);
                using Function = decltype(continuation);
                STATIC_REQUIRE(!is_executor_v<Function>);
                STATIC_REQUIRE(!is_executor_v<Future>);
                STATIC_REQUIRE(is_future_like_v<Future>);
                using value_type = future_value_t<Future>;
                using lvalue_type = std::add_lvalue_reference_t<value_type>;
                using rvalue_type = std::add_rvalue_reference_t<value_type>;
                STATIC_REQUIRE(
                    std::is_same<
                        value_type,
                        detail::small_vector<cfuture<int>>>::value);
                STATIC_REQUIRE(
                    std::is_same<
                        lvalue_type,
                        std::add_lvalue_reference_t<
                            detail::small_vector<cfuture<int>>>>::value);
                STATIC_REQUIRE(
                    std::is_same<
                        rvalue_type,
                        std::add_rvalue_reference_t<
                            detail::small_vector<cfuture<int>>>>::value);
#ifdef __cpp_lib_is_invocable
                // Test with `std` to ensure our implementation matches
                STATIC_REQUIRE(std::is_invocable_v<Function, value_type>);
                STATIC_REQUIRE(std::is_invocable_v<Function, lvalue_type>);
                STATIC_REQUIRE(
                    std::is_same_v<
                        std::invoke_result_t<Function, lvalue_type>,
                        int>);
#endif
                STATIC_REQUIRE(detail::is_invocable_v<Function, value_type>);
                STATIC_REQUIRE(detail::is_invocable_v<Function, lvalue_type>);
                STATIC_REQUIRE(
                    detail::next_future_traits<
                        default_executor_type,
                        std::decay_t<decltype(continuation)>,
                        std::decay_t<decltype(f)>>::is_valid);
                STATIC_REQUIRE(
                    detail::next_future_traits<
                        default_executor_type,
                        std::decay_t<decltype(continuation)>,
                        std::decay_t<decltype(f)>>::is_valid);
                STATIC_REQUIRE(is_future_like_v<Future>);
                STATIC_REQUIRE(!is_future_like_v<Function>);
                STATIC_REQUIRE(
                    detail::next_future_traits<
                        default_executor_type,
                        std::decay_t<decltype(continuation)>,
                        std::decay_t<decltype(f)>>::is_valid);
                auto f4 = then(f, continuation);
                REQUIRE(f4.get() == 2 + 3 + 4);
            }

            SECTION("Continue with const lvalue") {
                auto continuation =
                    [](detail::small_vector<cfuture<int>> const &) {
                    return 2 + 3 + 4; // <- cannot get from const future :P
                };
                STATIC_REQUIRE(is_future_like_v<decltype(f)>);
                STATIC_REQUIRE(!is_future_like_v<decltype(continuation)>);
                STATIC_REQUIRE(
                    detail::next_future_traits<
                        default_executor_type,
                        std::decay_t<decltype(continuation)>,
                        std::decay_t<decltype(f)>>::is_valid);
                auto f4 = then(f, continuation);
                REQUIRE(f4.get() == 2 + 3 + 4);
            }

            SECTION("Continue with rvalue") {
                auto continuation =
                    [](detail::small_vector<cfuture<int>> &&rs) {
                    return rs[0].get() + rs[1].get() + rs[2].get();
                };
                STATIC_REQUIRE(is_future_like_v<decltype(f)>);
                STATIC_REQUIRE(!is_future_like_v<decltype(continuation)>);
                STATIC_REQUIRE(
                    detail::next_future_traits<
                        default_executor_type,
                        std::decay_t<decltype(continuation)>,
                        std::decay_t<decltype(f)>>::is_valid);
                auto f4 = then(f, continuation);
                REQUIRE(f4.get() == 2 + 3 + 4);
            }
        }

        SECTION("Unwrap vector") {
            SECTION("Continue with value") {
                auto continuation = [](detail::small_vector<int> rs) {
                    return rs[0] + rs[1] + rs[2];
                };
                STATIC_REQUIRE(is_future_like_v<decltype(f)>);
                STATIC_REQUIRE(
                    detail::next_future_traits<
                        default_executor_type,
                        std::decay_t<decltype(continuation)>,
                        std::decay_t<decltype(f)>>::is_valid);
                SECTION("Sync unwrap") {
                    auto f4 = detail::future_continue(f, continuation);
                    REQUIRE(f4 == 2 + 3 + 4);
                }
                SECTION("Async continue") {
                    auto f4 = then(f, continuation);
                    REQUIRE(f4.get() == 2 + 3 + 4);
                }
            }

            SECTION("Continue with lvalue") {
                STATIC_REQUIRE(is_future_like_v<decltype(f)>);
                auto continuation = [](detail::small_vector<int> &rs) {
                    return rs[0] + rs[1] + rs[2];
                };
                static constexpr bool is_msvc =
#ifdef BOOST_MSVC
                    // this test might fail or succeed depending on the
                    // msvc version
                    true;
#else
                    false;
#endif
                constexpr bool const v = detail::next_future_traits<
                    default_executor_type,
                    std::decay_t<decltype(continuation)>,
                    std::decay_t<decltype(f)>>::is_valid;
                STATIC_REQUIRE(is_msvc || !v);
            }

            SECTION("Continue with const lvalue") {
                auto continuation = [](detail::small_vector<int> const &rs) {
                    return rs[0] + rs[1] + rs[2];
                };
                STATIC_REQUIRE(is_future_like_v<decltype(f)>);
                STATIC_REQUIRE(
                    detail::next_future_traits<
                        default_executor_type,
                        std::decay_t<decltype(continuation)>,
                        std::decay_t<decltype(f)>>::is_valid);
                SECTION("Sync unwrap") {
                    auto f4 = detail::future_continue(f, continuation);
                    REQUIRE(f4 == 2 + 3 + 4);
                }
                SECTION("Async continue") {
                    auto f4 = then(f, continuation);
                    REQUIRE(f4.get() == 2 + 3 + 4);
                }
            }
        }
    }

    SECTION("Range conjunction with lambdas") {
        std::function<int()> f1 = []() {
            return 2;
        };
        std::function<int()> f2 = []() {
            return 3;
        };
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
#ifdef __cpp_structured_bindings
            auto [r1, r2] = f.get();
            REQUIRE(r1.get() == 1);
            REQUIRE(r2.get() == 2);
#else
            std::tie(f1, f2) = f.get();
            REQUIRE(f1.get() == 1);
            REQUIRE(f2.get() == 2);
#endif
        }

        SECTION("Lambda conjunction") {
            // When a lambda is passed to when_all, it is converted into a
            // future immediately
            auto f = [] {
                return 1;
            } && [] {
                return 2;
            };
#ifdef __cpp_structured_bindings
            auto [r1, r2] = f.get();
#else
            cfuture<int> r1;
            cfuture<int> r2;
            std::tie(r1, r2) = f.get();
#endif
            REQUIRE(r1.get() == 1);
            REQUIRE(r2.get() == 2);
        }

        SECTION("Future/lambda conjunction") {
            cfuture<int> f1 = async([] { return 1; });
            auto f = f1 && [] {
                return 2;
            };
            REQUIRE_FALSE(f1.valid());
#ifdef __cpp_structured_bindings
            auto [r1, r2] = f.get();
#else
            cfuture<int> r1;
            cfuture<int> r2;
            std::tie(r1, r2) = f.get();
#endif
            REQUIRE(r1.get() == 1);
            REQUIRE(r2.get() == 2);
        }

        SECTION("Lambda/future conjunction") {
            cfuture<int> f2 = async([] { return 2; });
            auto f = [] {
                return 1;
            } && f2;
            REQUIRE_FALSE(f2.valid());
#ifdef __cpp_structured_bindings
            auto [r1, r2] = f.get();
#else
            cfuture<int> r1;
            cfuture<int> r2;
            std::tie(r1, r2) = f.get();
#endif
            REQUIRE(r1.get() == 1);
            REQUIRE(r2.get() == 2);
        }

        SECTION("Concatenate conjunctions") {
            cfuture<int> f1 = async([] { return 1; });
            cfuture<int> f2 = async([] { return 2; });
            cfuture<int> f3 = async([] { return 3; });
            auto f = f1 && f2 && f3 && []() {
                return 4;
            };
            REQUIRE_FALSE(f1.valid());
            REQUIRE_FALSE(f2.valid());
            REQUIRE_FALSE(f3.valid());
#ifdef __cpp_structured_bindings
            auto [r1, r2, r3, r4] = f.get();
#else
            cfuture<int> r1;
            cfuture<int> r2;
            cfuture<int> r3;
            cfuture<int> r4;
            std::tie(r1, r2, r3, r4) = f.get();
#endif
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
        auto f = f1 && f2 && f3 && []() {
            return 4;
        };
        auto c = f >> [](int a, int b, int c, int d) {
            return a + b + c + d;
        } >> [](int s) {
            return s * 2;
        };
        REQUIRE(c.get() == (1 + 2 + 3 + 4) * 2);
    }
}

#if defined(__GNUG__) && !defined(__clang__)
#    pragma GCC diagnostic pop
#endif