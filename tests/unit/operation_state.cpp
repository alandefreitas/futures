#include <futures/futures.hpp>
#include <array>
#include <catch2/catch.hpp>

template <typename T, typename T2>
void
set_value_or_void(futures::promise<T> &p, T2 test_value) {
    constexpr bool is_void_promise = std::is_same_v<T, void>;
    if constexpr (is_void_promise) {
        p.set_value();
    } else {
        p.set_value(test_value);
    }
}

template <class T, class T2 = T>
void
set_promise_tests(T2 test_value) {
    using namespace futures;
    static_assert(std::is_same_v<T, T2> || std::is_same_v<T, void>);
    promise<T> p;
    SECTION("Set value") {
        REQUIRE_NOTHROW(set_value_or_void(p, test_value));
        SECTION("Cannot set twice") {
            REQUIRE_THROWS(set_value_or_void(p, test_value));
        }
        SECTION("Cannot set exception after value") {
            REQUIRE_THROWS(p.set_exception(std::runtime_error("err")));
        }
        SECTION("Cannot set exception ptr after value") {
            REQUIRE_THROWS(p.set_exception(
                std::make_exception_ptr(std::runtime_error("err"))));
        }
    }

    SECTION("Set exception") {
        REQUIRE_NOTHROW(p.set_exception(std::runtime_error{ "" }));
        SECTION("Cannot set value") {
            REQUIRE_THROWS(set_value_or_void(p, test_value));
        }
        SECTION("Cannot set exception twice") {
            REQUIRE_THROWS(p.set_exception(std::runtime_error("err")));
        }
        SECTION("Cannot set exception ptr twice") {
            REQUIRE_THROWS(p.set_exception(
                std::make_exception_ptr(std::runtime_error("err"))));
        }
    }
}

template <class T, class T2 = T>
void
set_package_task_tests(T2 test_value) {
    using namespace futures;
    static_assert(std::is_same_v<T, T2> || std::is_same_v<T, void>);
    auto return_self = [](T2 n) {
        if constexpr (!std::is_same_v<T, void>) {
            return n;
        }
    };
    packaged_task<T(T2)> p(return_self);
    SECTION("Set value") {
        REQUIRE_NOTHROW(p(test_value));
        SECTION("Cannot set twice") {
            REQUIRE_THROWS(p(test_value));
        }
        SECTION("Can reset") {
            REQUIRE_NOTHROW(p.reset());
            REQUIRE_NOTHROW(p(test_value));
            REQUIRE_THROWS(p(test_value));
        }
    }
}

TEST_CASE(TEST_CASE_PREFIX "Shared state") {
    using namespace futures;

    SECTION("Promise") {
        SECTION("Concrete promise shared state") {
            SECTION("Int") {
                constexpr uint8_t test_number = 2;
                ::set_promise_tests<uint8_t>(test_number);
            }
            SECTION("Char") {
                ::set_promise_tests<char>('c');
            }
            SECTION("Void") {
                ::set_promise_tests<void>(std::nullptr_t{});
            }
        }

        SECTION("Future options") {
            promise<int, future_options<>> p;
            vfuture<int> f = p.get_future();
            SECTION("Inline") {
                p.set_value(2);
                REQUIRE(f.get() == 2);
            }
            SECTION("Thread") {
                std::thread t([&p]() {
                    p.set_value(2);
                });
                REQUIRE(f.get() == 2);
                t.join();
            }
            SECTION("Executor") {
                asio::thread_pool t(1);
                asio::post(t, [&p]() {
                    p.set_value(2);
                });
                REQUIRE(f.get() == 2);
            }
        }
    }

    SECTION("Packaged task") {
        SECTION("Concrete promise shared state") {
            SECTION("Int") {
                constexpr uint8_t test_number = 2;
                ::set_package_task_tests<uint8_t>(test_number);
            }
            SECTION("Char") {
                ::set_package_task_tests<char>('c');
            }
            SECTION("Void") {
                ::set_package_task_tests<void>(std::nullptr_t{});
            }
        }

        SECTION("Future options") {
            packaged_task<int(), future_options<>> p([]() { return 2; });
            vfuture<int> f = p.get_future();
            SECTION("Inline") {
                p();
                REQUIRE(f.get() == 2);
            }
            SECTION("Thread") {
                std::thread t(std::move(p));
                REQUIRE(f.get() == 2);
                t.join();
            }
            SECTION("Executor") {
                asio::thread_pool t(1);
                asio::post(t, std::move(p));
                REQUIRE(f.get() == 2);
            }
        }
    }
}