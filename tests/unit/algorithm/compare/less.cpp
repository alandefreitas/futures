#include <futures/algorithm/compare/less.hpp>
//
#include <catch2/catch.hpp>

TEST_CASE("algorithm comparisons less") {
    using namespace futures;

    REQUIRE(less{}(1, 2));
    REQUIRE_FALSE(less{}(2, 2));
    REQUIRE_FALSE(less{}(2, 1));
}
