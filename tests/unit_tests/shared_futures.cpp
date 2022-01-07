#include <array>
#include <string>

#include <catch2/catch.hpp>

#include <futures/futures.h>

TEST_CASE(TEST_CASE_PREFIX "Shared futures") {
    using namespace futures;
    SECTION("Basic") {
        shared_cfuture<int> f1 = async([] { return 1; }).share();
        shared_cfuture<int> f2 = f1;
        REQUIRE(f1.get() == 1);
        REQUIRE(f2.get() == 1);
        // Get twice
        REQUIRE(f1.get() == 1);
        REQUIRE(f2.get() == 1);
    }
    SECTION("Shared stop token") {
        shared_jcfuture<int> f1 = async([](const stop_token &st) {
                                      int i = 0;
                                      while (not st.stop_requested()) {
                                          ++i;
                                      }
                                      return i;
                                  }).share();
        shared_jcfuture<int> f2 = f1;
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        f2.request_stop();
        REQUIRE(f1.get() > 0);
        REQUIRE(f2.get() > 0);
        // Get twice
        REQUIRE(f1.get() > 0);
        REQUIRE(f2.get() > 0);
    }
    SECTION("Shared continuation") {
        shared_jcfuture<int> f1 = async([](const stop_token &st) {
                                      int i = 0;
                                      while (not st.stop_requested()) {
                                          ++i;
                                      }
                                      return i;
                                  }).share();
        shared_jcfuture<int> f2 = f1;
        // f3 has no stop token because f2 is shared so another task might depend on it
        cfuture<int> f3 = then(f2, [](int i) { return i == 0 ? 0 : (1 + i % 2); });
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        REQUIRE_FALSE(is_ready(f1));
        REQUIRE_FALSE(is_ready(f2));
        REQUIRE_FALSE(is_ready(f3));
        f2.request_stop();
        shared_cfuture<int> f4 = f3.share();
        REQUIRE(f4.get() != 0);
        REQUIRE((f4.get() == 1 || f4.get() == 2));
    }
}