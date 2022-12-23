#include <futures/algorithm/all_of.hpp>
//
#include "test_unary_invoke.hpp"
#include <catch2/catch.hpp>
#include <array>
#include <numeric>
#include <vector>

TEST_CASE("algorithm all of") {
    using namespace futures;

    SECTION("Overloads") {
        std::vector<int> v(5000);
        std::iota(v.begin(), v.end(), 1);
        auto fun = [](int v) {
            return v < 5500;
        };
        test_unary_invoke(all_of, v, fun, true);
    }

#if defined(FUTURES_HAS_CONSTANT_EVALUATED) && defined(__cpp_lib_array_constexpr)
    SECTION("constexpr") {
        constexpr std::array<int, 5> a = { 1, 2, 3, 4, 5 };

        struct is_odd_fn {
            constexpr bool
            operator()(int x) {
                return x & 1;
            };
        };
        constexpr is_odd_fn is_odd;
        STATIC_REQUIRE(futures::all_of(a, is_odd) == false);
        STATIC_REQUIRE(futures::all_of(a.begin(), a.end(), is_odd) == false);
        futures::asio::thread_pool pool;
        auto ex = pool.executor();
        STATIC_REQUIRE(futures::all_of(ex, a, is_odd) == false);
        STATIC_REQUIRE(
            futures::all_of(ex, a.begin(), a.end(), is_odd) == false);
        constexpr auto p = futures::halve_partitioner(1);
        STATIC_REQUIRE(futures::all_of(p, a, is_odd) == false);
        STATIC_REQUIRE(futures::all_of(p, a.begin(), a.end(), is_odd) == false);
        STATIC_REQUIRE(futures::all_of(ex, p, a, is_odd) == false);
        STATIC_REQUIRE(
            futures::all_of(ex, p, a.begin(), a.end(), is_odd) == false);
    }
#endif
}
