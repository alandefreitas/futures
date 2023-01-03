#include <futures/algorithm/partitioner/thread_partitioner.hpp>
//
#include <futures/algorithm/partitioner/partitioner_for.hpp>
#include <futures/algorithm/traits/iterator.hpp>
#include <catch2/catch.hpp>

TEST_CASE("algorithm partitioner thread partitioner") {
    using namespace futures;

    std::vector<int> r = { 1, 2 };
    thread_partitioner p(1);
    STATIC_REQUIRE(
        is_partitioner_for_v<thread_partitioner, std::vector<int>::iterator>);
    STATIC_REQUIRE(
        is_partitioner_for_v<thread_partitioner, iterator_t<std::vector<int>>>);
    REQUIRE(
        p(r.begin(), r.end()) - r.begin()
        == (futures::hardware_concurrency() == 1) + 1);
}
