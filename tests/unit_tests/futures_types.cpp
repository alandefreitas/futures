#include <futures/futures.hpp>
#include <array>
#include <string>
#include <catch2/catch.hpp>

TEST_CASE(TEST_CASE_PREFIX "Futures types") {
    using namespace futures;

    constexpr int32_t thread_pool_replicates = 100;
    SECTION("Continuable") {
        for (int32_t i = 0; i < thread_pool_replicates; ++i) {
            auto fn = [] {
                return 2;
            };
            using Function = decltype(fn);
            STATIC_REQUIRE(
                !std::is_invocable_v<std::decay_t<Function>, stop_token>);
            STATIC_REQUIRE(
                std::is_same_v<detail::launch_result_t<decltype(fn)>, int>);
            auto r = async(fn);
            REQUIRE(r.valid());
            REQUIRE(r.get() == 2);
            REQUIRE_FALSE(r.valid());
        }
    }

    SECTION("Shared") {
        using future_options_t = future_options<continuable_opt>;
        using shared_state_options = detail::
            remove_future_option_t<shared_opt, future_options_t>;
        STATIC_REQUIRE(
            std::is_same_v<
                shared_state_options,
                future_options<continuable_opt>>);

        using shared_future_options_t
            = future_options<continuable_opt, shared_opt>;
        using shared_shared_state_options = detail::
            remove_future_option_t<shared_opt, shared_future_options_t>;
        STATIC_REQUIRE(
            std::is_same_v<
                shared_shared_state_options,
                future_options<continuable_opt>>);

        auto fn = [] {
            return 2;
        };
        STATIC_REQUIRE(
            std::is_same_v<detail::launch_result_t<decltype(fn)>, int>);
        for (int32_t i = 0; i < thread_pool_replicates; ++i) {
            cfuture<int32_t> r = async(fn);
            REQUIRE(r.valid());
            shared_cfuture<int32_t> r2 = r.share();
            REQUIRE_FALSE(r.valid());
            REQUIRE(r2.valid());
            REQUIRE(r2.get() == 2);
            REQUIRE(r2.valid());
        }
    }

    SECTION("Promise / event future") {
        for (int32_t i = 0; i < thread_pool_replicates; ++i) {
            promise<int32_t> p;
            cfuture<int32_t> r = p.get_future();
            REQUIRE_FALSE(is_ready(r));
            p.set_value(2);
            REQUIRE(is_ready(r));
            REQUIRE(r.get() == 2);
        }
    }
}
