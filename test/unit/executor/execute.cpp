#include <futures/executor/execute.hpp>
//
#include <futures/executor/default_executor.hpp>
#include <futures/executor/inline_executor.hpp>
#include <futures/executor/thread_pool.hpp>
#include <catch2/catch.hpp>

#ifdef FUTURES_HAS_STANDALONE_ASIO
#    include <asio/thread_pool.hpp>
#    define FUTURES_HAS_EXTERNAL_ASIO
#elif FUTURES_HAS_BOOST
#    include <boost/asio/thread_pool.hpp>
#    define FUTURES_HAS_EXTERNAL_ASIO
#endif

TEST_CASE("executor execute") {
    using namespace futures;

    SECTION("Executor") {
        int a = 0;
        execute(inline_executor(), [&a] { ++a; });
        REQUIRE(a == 1);
    }

    SECTION("Executor context") {
        thread_pool p;
        int a = 0;
        execute(p, [&a] { ++a; });
        p.join();
        REQUIRE(a == 1);
    }

    SECTION("Executor context") {
        thread_pool p;
        int a = 0;
        execute(p, [&a] { ++a; });
        p.join();
        REQUIRE(a == 1);
    }

#ifdef FUTURES_HAS_EXTERNAL_ASIO
    SECTION("Asio Executor") {
#    ifdef FUTURES_HAS_STANDALONE_ASIO
        ::asio::thread_pool p;
#    elif FUTURES_HAS_BOOST
        ::boost::asio::thread_pool p;
#    endif
        int a = 0;
        execute(p.get_executor(), [&a] { ++a; });
        p.join();
        REQUIRE(a == 1);
    }
#endif
}
