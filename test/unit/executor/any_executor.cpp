#include <futures/executor/any_executor.hpp>
//
#include <futures/executor/execute.hpp>
#include <futures/executor/inline_executor.hpp>
#include <catch2/catch.hpp>

TEST_CASE("executor any executor") {
    using namespace futures;

    SECTION("any_executor()") {
        STATIC_REQUIRE(std::is_default_constructible<any_executor>::value);
        any_executor a;
        int v = 0;
        auto f = [&v] {
            ++v;
        };
        execute(a, f);
        REQUIRE(v == 1);
    }

    SECTION("any_executor(any_executor const&)") {
        STATIC_REQUIRE(std::is_copy_constructible<any_executor>::value);
        any_executor a(inline_executor{});
        any_executor b(a);
        int v = 0;
        execute(b, [&v] { ++v; });
        REQUIRE(v == 1);
    }

    SECTION("any_executor(any_executor&&)") {
        STATIC_REQUIRE(std::is_nothrow_move_constructible<any_executor>::value);
        any_executor a(inline_executor{});
        any_executor b(std::move(a));
        int v = 0;
        execute(b, [&v] { ++v; });
        REQUIRE(v == 1);
    }

    SECTION("any_executor(E)") {
        any_executor a(inline_executor{});
        int v = 0;
        execute(a, [&v] { ++v; });
        REQUIRE(v == 1);
    }

    SECTION("operator=(any_executor const&)") {
        any_executor a(inline_executor{});
        any_executor b;
        b = a;
        int v = 0;
        execute(b, [&v] { ++v; });
        REQUIRE(v == 1);
    }

    SECTION("operator=(any_executor&&)") {
        any_executor a(inline_executor{});
        any_executor b;
        b = std::move(a);
        int v = 0;
        execute(b, [&v] { ++v; });
        REQUIRE(v == 1);
    }

    SECTION("operator=(std::nullptr_t)") {
        any_executor b;
        b = nullptr;
        int v = 0;
        execute(b, [&v] { ++v; });
        REQUIRE(v == 1);
    }

    SECTION("operator=(E)") {
        any_executor b;
        b = inline_executor{};
        int v = 0;
        execute(b, [&v] { ++v; });
        REQUIRE(v == 1);
    }

    SECTION("execute(F&&)") {
        any_executor a(inline_executor{});
        int v = 0;
        auto f = [&v] {
            ++v;
        };
        SECTION("copy") {
            execute(a, f);
        }
        SECTION("move") {
            execute(a, std::move(f));
        }
        REQUIRE(v == 1);
    }
}
