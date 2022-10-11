#include <futures/adaptor/detail/continue.hpp>
//
#include <futures/future.hpp>
#include <futures/adaptor/when_any.hpp>
#include <futures/traits/future_value.hpp>
#include <catch2/catch.hpp>

#define REQUIRE_SAME(A, B) STATIC_REQUIRE(std::is_same_v<A, B>)

TEST_CASE(TEST_CASE_PREFIX "Continuation traits") {
    using namespace futures;
    using namespace futures::detail;
    SECTION("LR-values") {
        /*
         * We cannot invoke continuations with f(x) or f(std::move(x))
         * because of lifetime issues: if f takes a `&x`, then x
         * would be a reference to a value that lives where?
         *
         * In continue traits, because we always move the values to
         * continuations, we should check if the function is invocable
         * with std::add_rvalue_reference_t<std::decay<...>> to test
         * whether f(std::move(x)) is valid. This is the only test that will
         * cover functions that take S, S&&, and S const& unambiguously.
         */
        using S = std::string;
        using SL = std::add_lvalue_reference_t<std::string>;
        using SR = std::add_rvalue_reference_t<std::string>;
        using SCL = std::add_const_t<std::add_lvalue_reference_t<std::string>>;

        // invoke with f(std::move(x))
        auto f = [](std::string x) {
            return x;
        };
        using F = decltype(f);
        STATIC_REQUIRE(std::is_invocable_v<F, S>);
        STATIC_REQUIRE(std::is_invocable_v<F, SL>);
        STATIC_REQUIRE(std::is_invocable_v<F, SR>);
        STATIC_REQUIRE(std::is_invocable_v<F, SCL>);

        // invoke with f(x)
        auto fl = [](std::string& x) {
            return x;
        };
        using FL = decltype(fl);
        // True on MSVC. False on GCC:
        static constexpr bool is_msvc =
#ifdef BOOST_MSVC
            true;
#else
            false;
#endif
        STATIC_REQUIRE(
            (is_msvc && std::is_invocable_v<FL, S>)
            || (!is_msvc && !std::is_invocable_v<FL, S>) );
        STATIC_REQUIRE(std::is_invocable_v<FL, SL>);
        STATIC_REQUIRE(
            (is_msvc && std::is_invocable_v<FL, SR>)
            || (!is_msvc && !std::is_invocable_v<FL, SR>) );
        STATIC_REQUIRE(std::is_invocable_v<FL, SCL>);

        auto fr = [](std::string&& x) {
            return std::move(x);
        };
        using FR = decltype(fr);
        STATIC_REQUIRE(std::is_invocable_v<FR, S>);
        STATIC_REQUIRE_FALSE(std::is_invocable_v<FR, SL>);
        STATIC_REQUIRE(std::is_invocable_v<FR, SR>);
        STATIC_REQUIRE_FALSE(std::is_invocable_v<FR, SCL>);

        auto fcl = [](std::string const& x) {
            return x;
        };
        using FCL = decltype(fcl);
        STATIC_REQUIRE(std::is_invocable_v<FCL, S>);
        STATIC_REQUIRE(std::is_invocable_v<FCL, SL>);
        STATIC_REQUIRE(std::is_invocable_v<FCL, SR>);
        STATIC_REQUIRE(std::is_invocable_v<FCL, SCL>);
    }

    SECTION("Traits") {
        SECTION("Unwrap single value") {
            using F = future<int>;
            REQUIRE_SAME(unwrap_future_t<F>, int);
            using FF = future<future<int>>;
            REQUIRE_SAME(unwrap_future_t<FF>, int);
            using FFF = future<future<future<int>>>;
            REQUIRE_SAME(unwrap_future_t<FFF>, int);

            auto f = [](int32_t count) {
                return static_cast<double>(count) * 1.2;
            };
            using tag = continue_tag_t<F, decltype(f)>;
            REQUIRE_SAME(tag, continue_tags::rvalue_unwrap);
        }

        SECTION("Unwrap when any") {
            auto f = [](int32_t count) {
                return static_cast<double>(count) * 1.2;
            };
            using F = future<int32_t>;
            using T = std::tuple<F, F>;
            using WA = when_any_result<T>;
            using FWA = future<WA>;
            using tag = continue_tag_t<FWA, decltype(f)>;
            using exp = continue_tags::when_any_tuple_double_unwrap;
            REQUIRE_SAME(tag, exp);
        }
    }
}
