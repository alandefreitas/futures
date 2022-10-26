#include <futures/algorithm/find.hpp>
//
#include "test_value_cmp.hpp"
#include <catch2/catch.hpp>
#include <numeric>
#include <vector>

TEST_CASE("algorithm find") {
    using namespace futures;

    SECTION("Overloads") {
        std::vector<int> v(5000);
        std::iota(v.begin(), v.end(), 1);
        test_value_cmp(find, v, 2700, v.begin() + 2699);
    }
}
