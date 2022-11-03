#include <futures/algorithm/compare/compare_three_way.hpp>
//
#include <futures/algorithm/compare/strong_ordering.hpp>
#include <catch2/catch.hpp>

TEST_CASE("algorithm comparisons compare three way") {
    using namespace futures;

    REQUIRE(compare_three_way{}(1, 2) == strong_ordering::less);
    REQUIRE(compare_three_way{}(2, 2) == strong_ordering::equal);
    REQUIRE(compare_three_way{}(2, 1) == strong_ordering::greater);
}
