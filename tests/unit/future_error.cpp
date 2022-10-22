#include <futures/future_error.hpp>
//
#include <futures/launch.hpp>
#include <futures/adaptor/then.hpp>
#include <catch2/catch.hpp>
#include <array>
#include <string>

TEST_CASE("Exceptions") {
    using namespace futures;
    SECTION("Basic") {
        auto f1 = async([] { throw std::runtime_error("error"); });
        f1.wait();
        bool caught = false;
        try {
            f1.get();
        }
        catch (...) {
            caught = true;
        }
        REQUIRE(caught);
    }
    SECTION("Continuations") {
        auto f1 = async([] { throw std::runtime_error("error"); });
        auto f2 = then(f1, []() { return; });
        f2.wait();
        bool caught = false;
        try {
            f2.get();
        }
        catch (std::runtime_error&) {
            caught = true;
        }
        REQUIRE(caught);
    }
}
