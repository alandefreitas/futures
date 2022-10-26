#include <futures/algorithm/comparisons/compare_three_way.hpp>
//
#include <catch2/catch.hpp>

TEST_CASE("algorithm comparisons compare three way") {
    using namespace futures;

    REQUIRE(compare_three_way{}(1, 2) == -1);
    REQUIRE(compare_three_way{}(2, 2) == 0);
    REQUIRE(compare_three_way{}(2, 1) == 1);
}
