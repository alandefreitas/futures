#include <futures/futures.hpp>
#include <array>
#include <string>
#include <catch2/catch.hpp>

TEST_CASE(TEST_CASE_PREFIX "Disjunction") {
    using namespace futures;
    SECTION("Empty disjunction") {
        auto f = when_any();
        REQUIRE(f.valid());
        REQUIRE_NOTHROW(f.wait());
        REQUIRE(
            f.wait_for(std::chrono::seconds(0)) == std::future_status::ready);
        REQUIRE(
            f.wait_until(
                std::chrono::system_clock::now() + std::chrono::seconds(0))
            == std::future_status::ready);
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
        REQUIRE(
            f.wait_for(std::chrono::seconds(0)) == std::future_status::ready);
        REQUIRE(
            f.wait_until(
                std::chrono::system_clock::now() + std::chrono::seconds(0))
            == std::future_status::ready);
        REQUIRE(is_ready(f));
        when_any_result r = f.get();
        REQUIRE(r.index == size_t(0));
        REQUIRE(std::get<0>(r.tasks).get() == 2);
    }

    SECTION("Tuple disjunction") {
        cfuture<int> f1 = async([]() { return 2; });
        cfuture<double> f2 = async([]() { return 3.5; });
        cfuture<std::string> f3 = async([]() -> std::string { return "name"; });
        when_any_future<
            std::tuple<cfuture<int>, cfuture<double>, cfuture<std::string>>>
            f = when_any(f1, f2, f3);
        REQUIRE(f.valid());
        REQUIRE_FALSE(f1.valid());
        REQUIRE_FALSE(f2.valid());
        REQUIRE_FALSE(f3.valid());

        SECTION("Wait") {
            f.wait();
            std::future_status s1 = f.wait_for(std::chrono::seconds(0));
            REQUIRE(s1 == std::future_status::ready);
            std::future_status s2 = f.wait_until(
                std::chrono::system_clock::now() + std::chrono::seconds(0));
            REQUIRE(s2 == std::future_status::ready);
            REQUIRE(is_ready(f));
            when_any_result any_r = f.get();
            size_t i = any_r.index;
            auto [r1, r2, r3] = std::move(any_r.tasks);
            REQUIRE(i < 3);
            if (0 == i) {
                REQUIRE(r1.get() == 2);
            } else if (1 == i) {
                REQUIRE(r2.get() > 3.0);
            } else {
                REQUIRE(r3.get() == "name");
            }
        }

        SECTION("Continue") {
            auto continuation =
                [](when_any_result<std::tuple<
                       cfuture<int>,
                       cfuture<double>,
                       cfuture<std::string>>> r) {
                if (r.index == 0) {
                    return std::get<0>(r.tasks).get();
                } else if (r.index == 1) {
                    return static_cast<int>(std::get<1>(r.tasks).get());
                } else if (r.index == 2) {
                    return static_cast<int>(std::get<2>(r.tasks).get().size());
                }
                return 0;
            };
            using Future = decltype(f);
            using Function = decltype(continuation);
            STATIC_REQUIRE(detail::is_when_any_future_v<Future>);
            STATIC_REQUIRE(detail::is_valid_continuation_v<Function, Future>);
            auto f4 = then(f, continuation);
            int r = f4.get();
            REQUIRE_FALSE(r == 0);
            REQUIRE((r == 2 || r == 3 || r == 4));
        }

        SECTION("Unwrap to tuple of futures") {
            auto continuation =
                [](size_t index,
                   std::tuple<cfuture<int>, cfuture<double>, cfuture<std::string>>
                       tasks) {
                if (index == 0) {
                    return std::get<0>(tasks).get();
                } else if (index == 1) {
                    return static_cast<int>(std::get<1>(tasks).get());
                } else if (index == 2) {
                    return static_cast<int>(std::get<2>(tasks).get().size());
                }
                return 0;
            };
            using tuple_type = std::
                tuple<cfuture<int>, cfuture<double>, cfuture<std::string>>;
            using result_type = when_any_result<tuple_type>;
            STATIC_REQUIRE(detail::is_when_any_result_v<result_type>);
            SECTION("Sync unwrapping") {
                int r = detail::unwrap_and_continue(f, continuation);
                REQUIRE_FALSE(r == 0);
                REQUIRE((r == 2 || r == 3 || r == 4));
            }
            SECTION("Async continuation") {
                auto f4 = then(f, continuation);
                int r = f4.get();
                REQUIRE_FALSE(r == 0);
                REQUIRE((r == 2 || r == 3 || r == 4));
            }
        }

        SECTION("Unwrap to futures") {
            auto continuation =
                [](size_t index,
                   cfuture<int> f1,
                   cfuture<double> f2,
                   cfuture<std::string> f3) {
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
        // We can unwrap disjuntion results to a single value only when all
        // futures have the same type
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
            auto continuation = [](cfuture<int> r) {
                return r.get() * 3;
            };
            STATIC_REQUIRE(is_future_v<decltype(f)>);
            STATIC_REQUIRE(
                detail::is_valid_continuation_v<
                    decltype(continuation),
                    decltype(f)>);
            auto f4 = then(f, continuation);
            int r = f4.get();
            REQUIRE((r == 6 || r == 9 || r == 12));
        }

        SECTION("Unwrap to common value type") {
            // We can unwrap that here because all futures return int
            auto continuation = [](int r) {
                return r * 3;
            };
            using Future = decltype(f);
            using Function = decltype(continuation);
            STATIC_REQUIRE(is_future_v<decltype(f)>);
            // when_any_result -> int
            using value_type = unwrap_future_t<Future>;
            using when_any_sequence = typename value_type::sequence_type;
            using when_any_element_type = detail::range_or_tuple_element_type_t<
                when_any_sequence>;
            STATIC_REQUIRE(
                detail::is_tuple_invocable_v<
                    Function,
                    detail::tuple_type_concat_t<
                        std::tuple<>,
                        std::tuple<unwrap_future_t<when_any_element_type>>>>);
            STATIC_REQUIRE(
                std::is_same_v<
                    detail::unwrap_traits<Function, Future>::
                        unwrap_result_no_token_type,
                    int>);
            STATIC_REQUIRE(
                std::is_same_v<
                    detail::unwrap_traits<Function, Future>::
                        unwrap_result_with_token_type,
                    detail::unwrapping_failure_t>);
            STATIC_REQUIRE(detail::unwrap_traits<Function, Future>::
                               is_valid_without_stop_token);
            STATIC_REQUIRE(!detail::unwrap_traits<Function, Future>::
                               is_valid_with_stop_token);
            STATIC_REQUIRE(detail::unwrap_traits<Function, Future>::is_valid);
            STATIC_REQUIRE(detail::is_valid_continuation_v<Function, Future>);
            auto f4 = then(f, continuation);
            int r = f4.get();
            REQUIRE((r == 6 || r == 9 || r == 12));
        }
    }

    SECTION("Tuple disjunction with lambdas") {
        auto f1 = async([]() { return 2; });
        auto f2 = []() {
            return 3.5;
        };
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
            REQUIRE_NOTHROW(
                f.wait_for(std::chrono::seconds(0))
                == std::future_status::ready);
            REQUIRE_NOTHROW(
                f.wait_until(
                    std::chrono::system_clock::now() + std::chrono::seconds(0))
                == std::future_status::ready);
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
            auto continuation =
                [](when_any_result<detail::small_vector<cfuture<int>>> r) {
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
            STATIC_REQUIRE(
                detail::is_valid_continuation_v<
                    decltype(continuation),
                    decltype(f)>);
            auto f4 = then(f, continuation);
            int r = f4.get();
            REQUIRE_FALSE(r == 0);
            REQUIRE((r == 2 || r == 3 || r == 4));
        }

        SECTION("Unwrap to tuple of futures") {
            auto continuation =
                [](size_t index, detail::small_vector<cfuture<int>> tasks) {
                if (index == 0) {
                    return tasks[0].get();
                } else if (index == 1) {
                    return tasks[1].get();
                } else if (index == 2) {
                    return tasks[2].get();
                }
                return 0;
            };
            using tuple_type = detail::small_vector<cfuture<int>>;
            using result_type = when_any_result<tuple_type>;
            STATIC_REQUIRE(detail::is_when_any_result_v<result_type>);
            auto f4 = then(f, continuation);
            int r = f4.get();
            REQUIRE_FALSE(r == 0);
            REQUIRE((r == 2 || r == 3 || r == 4));
        }

        SECTION("Unwrap to common future type") {
            // We can unwrap that here because all vector futures return int
            auto continuation = [](cfuture<int> r) {
                return r.get() * 3;
            };
            auto f4 = then(f, continuation);
            int r = f4.get();
            REQUIRE((r == 6 || r == 9 || r == 12));
        }

        SECTION("Unwrap to common value type") {
            // We can unwrap that here because all futures return int
            auto continuation = [](int r) {
                return r * 3;
            };
            STATIC_REQUIRE(is_future_v<decltype(f)>);
            STATIC_REQUIRE(
                detail::is_valid_continuation_v<
                    decltype(continuation),
                    decltype(f)>);
            auto f4 = then(f, continuation);
            int r = f4.get();
            REQUIRE((r == 6 || r == 9 || r == 12));
        }
    }

    SECTION("Range disjunction with lambdas") {
        std::function<int()> f1 = []() {
            return 2;
        };
        std::function<int()> f2 = []() {
            return 3;
        };
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
            // When a lambda is passed to when_all, it is converted into a
            // future immediately
            auto f = [] {
                return 1;
            } || [] {
                return 2;
            };
            auto [index, tasks] = f.get();
            if (index == 0) {
                REQUIRE(std::get<0>(tasks).get() == 1);
            } else {
                REQUIRE(std::get<1>(tasks).get() == 2);
            }
        }

        SECTION("Future/lambda disjunction") {
            cfuture<int> f1 = async([] { return 1; });
            auto f = f1 || [] {
                return 2;
            };
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
            auto f = [] {
                return 1;
            } || f2;
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
            auto f = f1 || f2 || f3 || []() {
                return 4;
            };
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
        using Tuple = std::
            tuple<cfuture<int>, cfuture<int>, cfuture<int>, cfuture<int>>;
        using Future = when_any_future<Tuple>;
        Future f = f1 || f2 || f3 || []() {
            return 4;
        };
        STATIC_REQUIRE(
            std::is_same_v<unwrap_future_t<Future>, when_any_result<Tuple>>);
        SECTION("Sync unwrap") {
            detail::unwrap_and_continue(f, [](int a) { return a * 5; });
        }
        SECTION("Async continue") {
            auto c = f >> [](int a) {
                return a * 5;
            } >> [](int s) {
                return s * 5;
            };
            int r = c.get();
            REQUIRE(r >= 25);
            REQUIRE(r <= 100);
        }
    }
}
