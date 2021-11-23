#include <array>
#include <string>

#include <catch2/catch.hpp>

#include <futures/futures.h>

TEST_CASE("Future types") {
    using namespace futures;

    constexpr int thread_pool_replicates = 100;
    SECTION("Continuable") {
        for (int i = 0; i < thread_pool_replicates; ++i) {
            auto fn = [] { return 2; };
            using Function = decltype(fn);
            STATIC_REQUIRE(not std::is_invocable_v<std::decay_t<Function>, stop_token>);
            STATIC_REQUIRE(std::is_same_v<detail::async_future_result_of<Function>, cfuture<int>>);
            cfuture<int> r = async([] { return 2; });
            REQUIRE(r.valid());
            REQUIRE(r.get() == 2);
            REQUIRE_FALSE(r.valid());
        }
    }

    SECTION("Shared") {
        for (int i = 0; i < thread_pool_replicates; ++i) {
            cfuture<int> r = async([] { return 2; });
            REQUIRE(r.valid());
            shared_cfuture<int> r2 = r.share();
            REQUIRE_FALSE(r.valid());
            REQUIRE(r2.valid());
            REQUIRE(r2.get() == 2);
            REQUIRE(r2.valid());
        }
    }

    SECTION("Dispatch immediately") {
        for (int i = 0; i < thread_pool_replicates; ++i) {
            cfuture<int> r = async(launch::executor_now, [] { return 2; });
            REQUIRE(r.valid());
            REQUIRE(r.get() == 2);
            REQUIRE_FALSE(r.valid());
        }
    }

    SECTION("Promise / event future") {
        for (int i = 0; i < thread_pool_replicates; ++i) {
            std::promise<int> p;
            future<int> r = p.get_future();
            REQUIRE_FALSE(is_ready(r));
            p.set_value(2);
            REQUIRE(is_ready(r));
            REQUIRE(r.get() == 2);
        }
    }
}
