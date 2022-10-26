#include <futures/algorithm/comparisons/greater.hpp>
//
#include <catch2/catch.hpp>

TEST_CASE("algorithm comparisons greater") {
    using namespace futures;

    REQUIRE_FALSE(greater{}(1, 2));
    REQUIRE_FALSE(greater{}(2, 2));
    REQUIRE(greater{}(2, 1));
}
