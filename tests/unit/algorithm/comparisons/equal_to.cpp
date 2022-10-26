#include <futures/algorithm/comparisons/equal_to.hpp>
//
#include <catch2/catch.hpp>

TEST_CASE("algorithm comparisons equal to") {
    using namespace futures;

    REQUIRE_FALSE(equal_to{}(1, 2));
    REQUIRE(equal_to{}(2, 2));
    REQUIRE_FALSE(equal_to{}(2, 1));
}
