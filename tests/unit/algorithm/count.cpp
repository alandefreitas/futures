#include <futures/algorithm/count.hpp>
//
#include "test_value_cmp.hpp"
#include <catch2/catch.hpp>
#include <array>
#include <numeric>
#include <vector>

TEST_CASE("algorithm count") {
    using namespace futures;

    SECTION("Overloads") {
        std::vector<int> v(5000);
        std::iota(v.begin(), v.end(), 1);
        test_value_cmp(count, v, 2000, 1);
    }

#ifdef FUTURES_HAS_CONSTANT_EVALUATED
    SECTION("constexpr") {
        constexpr std::array<int, 5> a = { 1, 2, 3, 4, 5 };
        STATIC_REQUIRE(futures::count(a, 3) == 1);
        STATIC_REQUIRE(futures::count(a.begin(), a.end(), 3) == 1);
        futures::asio::thread_pool pool;
        auto ex = pool.executor();
        STATIC_REQUIRE(futures::count(ex, a, 3) == 1);
        STATIC_REQUIRE(futures::count(ex, a.begin(), a.end(), 3) == 1);
        constexpr auto p = futures::halve_partitioner(1);
        STATIC_REQUIRE(futures::count(p, a, 3) == 1);
        STATIC_REQUIRE(futures::count(p, a.begin(), a.end(), 3) == 1);
        STATIC_REQUIRE(futures::count(ex, p, a, 3) == 1);
        STATIC_REQUIRE(futures::count(ex, p, a.begin(), a.end(), 3) == 1);
    }
#endif
}
