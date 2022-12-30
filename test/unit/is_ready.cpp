#include <futures/is_ready.hpp>
//
#include <futures/launch.hpp>
#include <catch2/catch.hpp>
#include <future>

#ifdef FUTURES_HAS_BOOST
#    include <boost/fiber/future.hpp>
#endif

TEST_CASE("is ready") {
    using namespace futures;

    SECTION("Futures") {
        auto f = futures::async([]() { return 2; });
        f.wait();
        REQUIRE(f.is_ready());
        REQUIRE(is_ready(f));
        REQUIRE(f.get() == 2);
    }

    SECTION("Std") {
        auto f = std::async([]() { return 2; });
        f.wait();
        REQUIRE(is_ready(f));
        REQUIRE(f.get() == 2);
    }
}
