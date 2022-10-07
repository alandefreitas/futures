#include <futures/futures.hpp>
#include <array>
#include <string>
#include <catch2/catch.hpp>

TEST_CASE(TEST_CASE_PREFIX "Asio default executors") {
    using namespace futures;

    SECTION("Wait and stop") {
        asio::thread_pool pool(1);
        asio::thread_pool::executor_type ex = pool.executor();
        asio::thread_pool::executor_type ex2 = pool.executor();
        REQUIRE(&pool == &ex.context());
        REQUIRE(ex == ex2);
        int i = 0;
        asio::post(ex, [&]() { ++i; });
        asio::post(ex2, [&]() { ++i; });
        pool.wait(); // <- this will stop the pool
        REQUIRE(i == 2);
        asio::post(ex, [&]() { ++i; });
        pool.wait();
        REQUIRE(i == 2); // <- pool had already stopped
    }

    constexpr int thread_pool_replicates = 100;

    SECTION("Default thread pool") {
        asio::thread_pool &pool = default_execution_context();
        asio::thread_pool::executor_type ex = pool.executor();
        for (int i = 0; i < thread_pool_replicates; ++i) {
            auto f = asio::post(ex, asio::use_future([&i]() { return i * 2; }));
            REQUIRE(await(f) == i * 2);
        }
    }

    SECTION("Default executor") {
        asio::thread_pool::executor_type ex = make_default_executor();
        for (int i = 0; i < thread_pool_replicates; ++i) {
            auto f = asio::post(ex, asio::use_future([&i]() { return i * 3; }));
            REQUIRE(f.get() == i * 3);
        }
    }

    SECTION("Function precedence") {
        SECTION("Dispatch") {
            asio::thread_pool::executor_type ex = make_default_executor();
            for (int i = 0; i < thread_pool_replicates; ++i) {
                bool a = false;
                bool b = false;
                std::future<void> f1;
                std::future<void> f2;
                asio::post(ex, asio::use_future([&] {
                               f1 = asio::dispatch(ex, asio::use_future([&] {
                                                       a = true;
                                                   }));
                               f2 = asio::dispatch(ex, asio::use_future([&] {
                                                       b = true;
                                                   }));
                               REQUIRE(a);
                               REQUIRE(b);
                           }))
                    .wait();
                f1.wait();
                f2.wait();
            }
        }

        SECTION("Defer") {
            asio::thread_pool::executor_type ex = make_default_executor();
            for (int i = 0; i < thread_pool_replicates; ++i) {
                bool a = false;
                bool b = false;
                std::future<void> f1;
                std::future<void> f2;
                asio::post(ex, asio::use_future([&] {
                               f1 = asio::defer(ex, asio::use_future([&] {
                                                    a = true;
                                                }));
                               f2 = asio::defer(ex, asio::use_future([&] {
                                                    b = true;
                                                }));
                               REQUIRE_FALSE(a);
                               REQUIRE_FALSE(b);
                           }))
                    .wait();
                f1.wait();
                f2.wait();
            }
        }
    }
}
