#include <futures/executor.hpp>
//
#include <catch2/catch.hpp>
#include <array>

#if defined(FUTURES_USE_STANDALONE_ASIO)
#    include <asio/use_future.hpp>
#elif defined(FUTURES_USE_BOOST_ASIO)
#    include <boost/asio/use_future.hpp>
#else
#    define FUTURES_IGNORE_USE_FUTURE_TESTS
#endif

TEST_CASE("Asio default executors") {
    using namespace futures;

    SECTION("Wait and stop") {
        futures::asio::thread_pool pool(1);
        futures::asio::thread_pool::executor_type ex = pool.executor();
        futures::asio::thread_pool::executor_type ex2 = pool.executor();
        REQUIRE(&pool == &ex.context());
        REQUIRE(ex == ex2);
        int i = 0;
        futures::detail::execute(ex, [&]() { ++i; });
        futures::detail::execute(ex2, [&]() { ++i; });
        pool.wait(); // <- this will stop the pool
        REQUIRE(i == 2);
        futures::detail::execute(ex, [&]() { ++i; });
        pool.wait();
        REQUIRE(i == 2); // <- pool had already stopped
    }
}
