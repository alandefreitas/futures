#include <futures/algorithm/count_if.hpp>
//
#include "test_unary_invoke.hpp"
#include <catch2/catch.hpp>
#include <array>
#include <numeric>
#include <vector>

TEST_CASE("algorithm count if") {
    using namespace futures;

    SECTION("Overloads") {
        std::vector<int> v(5000);
        std::iota(v.begin(), v.end(), 1);
        auto fun = [](int v) {
            return v & 1;
        };
        test_unary_invoke(count_if, v, fun, 2500);
    }

#if defined(FUTURES_HAS_CONSTANT_EVALUATED) && defined(__cpp_lib_array_constexpr)
    SECTION("constexpr") {
        constexpr std::array<int, 5> a = { 1, 2, 3, 4, 5 };

        constexpr auto is_odd = [](int x) {
            return x & 1;
        };
        STATIC_REQUIRE(futures::count_if(a, is_odd) == 3);
        STATIC_REQUIRE(futures::count_if(a.begin(), a.end(), is_odd) == 3);
        futures::asio::thread_pool pool;
        auto ex = pool.executor();
        STATIC_REQUIRE(futures::count_if(ex, a, is_odd) == 3);
        STATIC_REQUIRE(futures::count_if(ex, a.begin(), a.end(), is_odd) == 3);
        constexpr auto p = futures::halve_partitioner(1);
        STATIC_REQUIRE(futures::count_if(p, a, is_odd) == 3);
        STATIC_REQUIRE(futures::count_if(p, a.begin(), a.end(), is_odd) == 3);
        STATIC_REQUIRE(futures::count_if(ex, p, a, is_odd) == 3);
        STATIC_REQUIRE(
            futures::count_if(ex, p, a.begin(), a.end(), is_odd) == 3);
    }
#endif
}
