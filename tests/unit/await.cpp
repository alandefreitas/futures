#include <futures/await.hpp>
//
#include <futures/launch.hpp>
#include <catch2/catch.hpp>

TEST_CASE("await") {
    using namespace futures;

    SECTION("Single future") {
        auto f = async([](){return 2;});
        auto v = await(f);
        REQUIRE(v == 2);
    }

    SECTION("Variadic") {
        auto f1 = async([](){return 2;});
        auto f2 = async([](){return 3;});
        auto v = await(f1, f2);
        REQUIRE(std::get<0>(v) == 2);
        REQUIRE(std::get<1>(v) == 3);
    }

    SECTION("Variadic with void") {
        auto f1 = async([](){return 2;});
        auto f2 = async([](){return;});
        auto v = await(f1, f2);
        REQUIRE(std::get<0>(v) == 2);
        STATIC_REQUIRE(std::tuple_size_v<decltype(v)> == 1);
    }

}
