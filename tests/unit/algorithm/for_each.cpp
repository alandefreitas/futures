#include <futures/algorithm/for_each.hpp>
//
#include "test_unary_invoke.hpp"
#include <catch2/catch.hpp>
#include <algorithm>
#include <atomic>
#include <vector>

TEST_CASE("algorithm for each") {
    using namespace futures;

    SECTION("Overloads") {
        // range
        std::vector<int> v(5000);
        std::iota(v.begin(), v.end(), 1);

        // callback
        std::atomic<int> c(0);
        auto fun = [&c](int v) {
            c.fetch_add(v, std::memory_order_release);
        };

        // check
        int const v_sum = std::accumulate(v.begin(), v.end(), 0, std::plus<>());
        auto check = [&c, v_sum] {
            int r = c.load(std::memory_order_acquire);
            REQUIRE(r == v_sum);
            c.store(0, std::memory_order_release);
        };

        test_void_unary_invoke(for_each, v, fun, check);
    }
}
