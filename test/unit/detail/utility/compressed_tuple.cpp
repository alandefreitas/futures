#include <futures/detail/utility/compressed_tuple.hpp>
//
#include <catch2/catch.hpp>

TEST_CASE("Compressed tuple") {
    using namespace futures::detail;
    compressed_tuple<int, double, double> ct;
    REQUIRE(ct.get<0>() == 0);
    ct.get<0>() = 2;
    int a = ct.get<0>();
    REQUIRE(a == 2);
    a = ct.get(mp_size_t<0>{});
    REQUIRE(a == 2);
    REQUIRE(ct.size() == 3);
    ct = make_tuple(1, 6.7, 7.2);
    REQUIRE(ct.get<0>() == 1);
}
