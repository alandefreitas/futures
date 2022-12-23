#include <futures/algorithm/partitioner/partitioner.hpp>
//
#include <catch2/catch.hpp>

TEST_CASE("algorithm partitioner partitioner") {
    using namespace futures;

    SECTION("Partitioners") {
        SECTION("halve_partitioner") {
            std::vector<int> r = { 1, 2 };
            halve_partitioner p(1);
            STATIC_REQUIRE(
                is_partitioner_v<halve_partitioner, std::vector<int>::iterator>);
            STATIC_REQUIRE(
                is_range_partitioner_v<halve_partitioner, std::vector<int>>);
            REQUIRE(p(r.begin(), r.end()) - r.begin() == 1);
            REQUIRE(p(r.begin(), r.end()) == r.begin() + 1);
        }

        SECTION("thread_partitioner") {
            std::vector<int> r = { 1, 2 };
            thread_partitioner p(1);
            STATIC_REQUIRE(
                is_partitioner_v<thread_partitioner, std::vector<int>::iterator>);
            STATIC_REQUIRE(
                is_range_partitioner_v<thread_partitioner, std::vector<int>>);
            REQUIRE(
                p(r.begin(), r.end()) - r.begin()
                == (futures::hardware_concurrency() == 1) + 1);
        }

        SECTION("default_partitioner") {
            std::vector<int> r = { 1, 2 };
            default_partitioner p(1);
            STATIC_REQUIRE(
                is_partitioner_v<
                    default_partitioner,
                    std::vector<int>::iterator>);
            STATIC_REQUIRE(
                is_range_partitioner_v<default_partitioner, std::vector<int>>);
            REQUIRE(
                p(r.begin(), r.end()) - r.begin()
                == (futures::hardware_concurrency() == 1) + 1);
        }
    }

    SECTION("Grain size") {
        REQUIRE(make_grain_size(64) >= 1);
        std::vector<int> r = { 1, 2 };
        REQUIRE(
            make_default_partitioner(64)(r.begin(), r.end()) - r.begin() == 2);
        REQUIRE(make_default_partitioner(64)(r.begin(), r.end()) == r.end());
    }
}
