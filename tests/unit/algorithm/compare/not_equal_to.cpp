#include <futures/algorithm/compare/not_equal_to.hpp>
//
#include <catch2/catch.hpp>

TEST_CASE("algorithm comparisons not equal to") {
    using namespace futures;

    REQUIRE(not_equal_to{}(1, 2));
    REQUIRE_FALSE(not_equal_to{}(2, 2));
    REQUIRE(not_equal_to{}(2, 1));
}
