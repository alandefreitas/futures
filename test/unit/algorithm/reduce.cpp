#include <futures/algorithm/reduce.hpp>
//
#include "test_binary_invoke.hpp"
#include <catch2/catch.hpp>
#include <array>

TEST_CASE("algorithm reduce") {
    using namespace futures;

    SECTION("Overloads") {
        std::vector<int> v(5000);
        std::iota(v.begin(), v.end(), 1);
        auto custom_plus = [](int a, int b) {
            return a + b;
        };
        int const v_sum = std::accumulate(v.begin(), v.end(), 0, std::plus<>());
        test_binary_invoke(reduce, v, custom_plus, v_sum);
    }

#ifdef FUTURES_HAS_CONSTANT_EVALUATED
    SECTION("constexpr") {
        constexpr std::array<int, 5> a = { 1, 2, 3, 4, 5 };
        STATIC_REQUIRE(futures::reduce(a) == 15);
        STATIC_REQUIRE(futures::reduce(a, 0) == 15);
        STATIC_REQUIRE(futures::reduce(a.begin(), a.end()) == 15);
        STATIC_REQUIRE(futures::reduce(a.begin(), a.end(), 0) == 15);

        futures::asio::thread_pool pool;
        auto ex = pool.executor();
        STATIC_REQUIRE(futures::reduce(ex, a) == 15);
        STATIC_REQUIRE(futures::reduce(ex, a, 0) == 15);
        STATIC_REQUIRE(futures::reduce(ex, a.begin(), a.end()) == 15);
        STATIC_REQUIRE(futures::reduce(ex, a.begin(), a.end(), 0) == 15);

        constexpr auto p = futures::halve_partitioner(1);
        STATIC_REQUIRE(futures::reduce(p, a) == 15);
        STATIC_REQUIRE(futures::reduce(p, a, 0) == 15);
        STATIC_REQUIRE(futures::reduce(p, a.begin(), a.end()) == 15);
        STATIC_REQUIRE(futures::reduce(p, a.begin(), a.end(), 0) == 15);

        STATIC_REQUIRE(futures::reduce(ex, p, a) == 15);
        STATIC_REQUIRE(futures::reduce(ex, p, a, 0) == 15);
        STATIC_REQUIRE(futures::reduce(ex, p, a.begin(), a.end()) == 15);
        STATIC_REQUIRE(futures::reduce(ex, p, a.begin(), a.end(), 0) == 15);
    }
#endif
}
