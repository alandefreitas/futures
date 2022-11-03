#include <futures/algorithm/compare/greater_equal.hpp>
//
#include <catch2/catch.hpp>

TEST_CASE("algorithm comparisons greater equal") {
    using namespace futures;

    REQUIRE_FALSE(greater_equal{}(1, 2));
    REQUIRE(greater_equal{}(2, 2));
    REQUIRE(greater_equal{}(2, 1));
}
