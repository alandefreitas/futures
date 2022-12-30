#include <futures/executor/thread_pool.hpp>
//
#include <futures/executor/execute.hpp>
#include <catch2/catch.hpp>

TEST_CASE("executor thread pool") {
    using namespace futures;

    SECTION("Constructor") {
        REQUIRE_NOTHROW(thread_pool{});
        REQUIRE_NOTHROW(thread_pool{ 4 });
        STATIC_REQUIRE(!std::is_move_constructible<thread_pool>::value);
    }

    SECTION("Executor") {
        SECTION("Directly") {
            thread_pool p;
            auto ex = p.get_executor();
            int a = 0;
            ex.execute([&a] { ++a; });
            p.join();
            REQUIRE(a == 1);
        }

        SECTION("On executor") {
            thread_pool p;
            int a = 0;
            execute(p.get_executor(), [&a] { ++a; });
            p.join();
            REQUIRE(a == 1);
        }

        SECTION("On context") {
            thread_pool p;
            int a = 0;
            execute(p, [&a] { ++a; });
            p.join();
            REQUIRE(a == 1);
        }
    }
}
