#include <futures/error.hpp>
//
#include <futures/launch.hpp>
#include <futures/adaptor/then.hpp>
#include <catch2/catch.hpp>
#include <array>
#include <string>

TEST_CASE("Exceptions") {
    using namespace futures;

    SECTION("Error types") {
        auto ok = [](auto&& e) {
            try {
                throw e;
            } catch (futures::error& e) {
                REQUIRE(e.code().value() != 0);
            }
        };
        ok(broken_promise{});
        ok(future_already_retrieved{});
        ok(promise_already_satisfied{});
        ok(no_state{});
        ok(promise_uninitialized{});
        ok(packaged_task_uninitialized{});
        ok(future_uninitialized{});
        ok(future_deferred{});
    }
}
