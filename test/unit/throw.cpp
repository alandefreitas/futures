#include <futures/throw.hpp>
//
#include <catch2/catch.hpp>
#ifdef __cpp_lib_source_location
#    include <source_location>
#endif

template <class T>
void
ignore_unused(T&) {}

TEST_CASE("throw") {
    using namespace futures;

    SECTION("throw_exception throws") {
#ifndef FUTURES_NO_EXCEPTIONS
        REQUIRE_THROWS_AS(throw_exception(std::exception{}), std::exception);
#endif
    }
}
