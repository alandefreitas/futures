#include <futures/algorithm/partitioner/default_partitioner.hpp>
//
#include <futures/algorithm/partitioner/partitioner_for.hpp>
#include <catch2/catch.hpp>

TEST_CASE("algorithm partitioner default partitioner") {
    using namespace futures;

    SECTION("Partitioner") {
        std::vector<int> r = { 1, 2 };
        default_partitioner p(1);
        STATIC_REQUIRE(
            is_partitioner_for_v<
                default_partitioner,
                std::vector<int>::iterator>);
        STATIC_REQUIRE(
            is_partitioner_for_v<
                default_partitioner,
                iterator_t<std::vector<int>>>);
        REQUIRE(
            p(r.begin(), r.end()) - r.begin()
            == (futures::hardware_concurrency() == 1) + 1);
    }

    SECTION("Grain size") {
        REQUIRE(make_grain_size(64) >= 1);
        std::vector<int> r = { 1, 2 };
        REQUIRE(
            make_default_partitioner(64)(r.begin(), r.end()) - r.begin() == 2);
        REQUIRE(make_default_partitioner(64)(r.begin(), r.end()) == r.end());
    }
}
