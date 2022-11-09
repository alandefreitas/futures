#include <futures/adaptor/make_ready_future.hpp>
//
#include <futures/is_ready.hpp>
#include <catch2/catch.hpp>

TEST_CASE("adaptor make ready future") {
    using namespace futures;
    SECTION("Value") {
        auto f = make_ready_future(3);
        REQUIRE(is_ready(f));
        REQUIRE(f.get() == 3);
    }

    SECTION("Reference") {
        int a = 3;
        auto f = make_ready_future(std::reference_wrapper(a));
        REQUIRE(is_ready(f));
        REQUIRE(f.get() == 3);
    }

    SECTION("Void") {
        auto f = make_ready_future();
        REQUIRE(is_ready(f));
        REQUIRE_NOTHROW(f.get());
    }

    SECTION("Exception ptr") {
        auto f = make_exceptional_future<int>(
            std::make_exception_ptr(std::logic_error("error")));
        REQUIRE(is_ready(f));
        REQUIRE_THROWS(f.get());
    }

    SECTION("Exception ptr") {
        auto f = make_exceptional_future<int>(std::logic_error("error"));
        REQUIRE(is_ready(f));
        REQUIRE_THROWS(f.get());
    }
}
