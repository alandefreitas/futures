#include <futures/algorithm/any_of.hpp>
//
#include "test_unary_invoke.hpp"
#include <catch2/catch.hpp>
#include <algorithm>
#include <array>
#include <vector>

TEST_CASE("algorithm any of") {
    using namespace futures;

    SECTION("Overloads") {
        std::vector<int> v(5000);
        std::iota(v.begin(), v.end(), 1);
        auto fun = [](int v) {
            return v == 2700;
        };
        test_unary_invoke(any_of, v, fun, true);
    }

#ifdef FUTURES_HAS_CONSTANT_EVALUATED
    SECTION("constexpr") {
        constexpr std::array<int, 5> a = { 1, 2, 3, 4, 5 };

        constexpr auto is_odd = [](int x) {
            return x & 1;
        };
        STATIC_REQUIRE(futures::any_of(a, is_odd) == true);
        STATIC_REQUIRE(futures::any_of(a.begin(), a.end(), is_odd) == true);
        futures::asio::thread_pool pool;
        auto ex = pool.executor();
        STATIC_REQUIRE(futures::any_of(ex, a, is_odd) == true);
        STATIC_REQUIRE(futures::any_of(ex, a.begin(), a.end(), is_odd) == true);
        constexpr auto p = futures::halve_partitioner(1);
        STATIC_REQUIRE(futures::any_of(p, a, is_odd) == true);
        STATIC_REQUIRE(futures::any_of(p, a.begin(), a.end(), is_odd) == true);
        STATIC_REQUIRE(futures::any_of(ex, p, a, is_odd) == true);
        STATIC_REQUIRE(
            futures::any_of(ex, p, a.begin(), a.end(), is_odd) == true);
    }
#endif
}
