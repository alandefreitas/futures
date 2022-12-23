#include <futures/algorithm/find_if_not.hpp>
//
#include "test_unary_invoke.hpp"
#include <catch2/catch.hpp>
#include <algorithm>
#include <array>
#include <numeric>
#include <vector>

TEST_CASE("algorithm find if not") {
    using namespace futures;

    SECTION("Overloads") {
        std::vector<int> v(5000);
        std::iota(v.begin(), v.end(), 1);
        auto fun = [](int v) {
            return v < 2700;
        };
        test_unary_invoke(find_if_not, v, fun, v.begin() + 2699);
    }

#if defined(FUTURES_HAS_CONSTANT_EVALUATED) \
    && defined(__cpp_lib_array_constexpr)
    SECTION("constexpr") {
        constexpr std::array<int, 5> a = { 1, 2, 3, 4, 5 };

        constexpr auto is_odd = [](int x) {
            return x & 1;
        };
        STATIC_REQUIRE(futures::find_if_not(a, is_odd) == std::next(a.begin()));
        STATIC_REQUIRE(
            futures::find_if_not(a.begin(), a.end(), is_odd)
            == std::next(a.begin()));
        futures::asio::thread_pool pool;
        auto ex = pool.executor();
        STATIC_REQUIRE(
            futures::find_if_not(ex, a, is_odd) == std::next(a.begin()));
        STATIC_REQUIRE(
            futures::find_if_not(ex, a.begin(), a.end(), is_odd)
            == std::next(a.begin()));
        constexpr auto p = futures::halve_partitioner(1);
        STATIC_REQUIRE(
            futures::find_if_not(p, a, is_odd) == std::next(a.begin()));
        STATIC_REQUIRE(
            futures::find_if_not(p, a.begin(), a.end(), is_odd)
            == std::next(a.begin()));
        STATIC_REQUIRE(
            futures::find_if_not(ex, p, a, is_odd) == std::next(a.begin()));
        STATIC_REQUIRE(
            futures::find_if_not(ex, p, a.begin(), a.end(), is_odd)
            == std::next(a.begin()));
    }
#endif
}
