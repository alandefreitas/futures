#include <futures/algorithm/comparisons/less_equal.hpp>
//
#include <catch2/catch.hpp>

TEST_CASE("algorithm comparisons less equal") {
    using namespace futures;

    REQUIRE(less_equal{}(1, 2));
    REQUIRE(less_equal{}(2, 2));
    REQUIRE_FALSE(less_equal{}(2, 1));
}
