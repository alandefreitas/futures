#include <array>
#include <string>

#include <catch2/catch.hpp>

#include <futures/futures.h>

TEST_CASE("Futures types") {
    using namespace futures;

    constexpr int32_t thread_pool_replicates = 100;
    SECTION("Continuable") {
        for (int32_t i = 0; i < thread_pool_replicates; ++i) {
            auto fn = [] { return 2; };
            using Function = decltype(fn);
            STATIC_REQUIRE(not std::is_invocable_v<std::decay_t<Function>, stop_token>);
            cfuture<int32_t> r = async([] { return 2; });
            REQUIRE(r.valid());
            REQUIRE(r.get() == 2);
            REQUIRE_FALSE(r.valid());
        }
    }

    SECTION("Shared") {
        for (int32_t i = 0; i < thread_pool_replicates; ++i) {
            cfuture<int32_t> r = async([] { return 2; });
            REQUIRE(r.valid());
            shared_cfuture<int32_t> r2 = r.share();
            REQUIRE_FALSE(r.valid());
            REQUIRE(r2.valid());
            REQUIRE(r2.get() == 2);
            REQUIRE(r2.valid());
        }
    }

    SECTION("Dispatch immediately") {
        for (int32_t i = 0; i < thread_pool_replicates; ++i) {
            cfuture<int32_t> r = async(launch::executor_now, [] { return 2; });
            REQUIRE(r.valid());
            REQUIRE(r.get() == 2);
            REQUIRE_FALSE(r.valid());
        }
    }

    SECTION("Promise / event future") {
        for (int32_t i = 0; i < thread_pool_replicates; ++i) {
            promise<int32_t> p;
            future<int32_t> r = p.template get_future<future<int32_t>>();
            REQUIRE_FALSE(is_ready(r));
            p.set_value(2);
            REQUIRE(is_ready(r));
            REQUIRE(r.get() == 2);
        }
    }
}
