#include <array>
#include <string>
#include <catch2/catch.hpp>
#include <futures/futures.hpp>
#if __has_include(<futures/detail/container/lock_free_queue.hpp>)
#    include <futures/detail/container/lock_free_queue.hpp>
#endif

TEST_CASE(TEST_CASE_PREFIX "Mt-Queue") {
    using namespace futures;
    using namespace futures::detail;

    SECTION("Trivial") {
        SECTION("Queue") {
            lock_free_queue<int> q;
            q.push(1);
            q.push(2);
            q.push(3);
            REQUIRE(q.pop() == 1);
            REQUIRE_FALSE(q.empty());
            REQUIRE(q.pop() == 2);
            REQUIRE_FALSE(q.empty());
            REQUIRE(q.pop() == 3);
            REQUIRE(q.empty());
        }
    }

    SECTION("Pointer") {
        SECTION("Queue") {
            lock_free_queue<int*> q;
            std::array<int, 3> a{};
            q.push(&a[0]);
            q.push(&a[1]);
            q.push(&a[2]);
            REQUIRE(q.pop() == &a[0]);
            REQUIRE_FALSE(q.empty());
            REQUIRE(q.pop() == &a[1]);
            REQUIRE_FALSE(q.empty());
            REQUIRE(q.pop() == &a[2]);
            REQUIRE(q.empty());
        }
    }

    SECTION("Future") {
        SECTION("Queue") {
            lock_free_queue<basic_future<int, future_options<>>> q;
            q.push(make_ready_future(1));
            q.push(make_ready_future(2));
            q.push(make_ready_future(3));
            REQUIRE(q.pop().get() == 1);
            REQUIRE_FALSE(q.empty());
            REQUIRE(q.pop().get() == 2);
            REQUIRE_FALSE(q.empty());
            REQUIRE(q.pop().get() == 3);
            REQUIRE(q.empty());
        }
    }
}
