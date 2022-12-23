#include <futures/detail/operation_state.hpp>
//
#include <futures/packaged_task.hpp>
#include <futures/promise.hpp>
#include <futures/detail/deps/asio/post.hpp>
#include <catch2/catch.hpp>
#include <array>

template <typename T, typename T2>
void
set_value_or_void_impl(
    std::true_type /* is void */,
    futures::promise<T> &p,
    T2 test_value) {
    boost::ignore_unused(test_value);
    p.set_value();
}

template <typename T, typename T2>
void
set_value_or_void_impl(
    std::false_type /* is void */,
    futures::promise<T> &p,
    T2 test_value) {
    p.set_value(test_value);
}

template <typename T, typename T2>
void
set_value_or_void(futures::promise<T> &p, T2 test_value) {
    set_value_or_void_impl(std::is_void<T>{}, p, test_value);
}

template <class T, class T2 = T>
void
set_promise_tests(T2 test_value) {
    using namespace futures;
    FUTURES_STATIC_ASSERT(
        std::is_same<T, T2>::value || std::is_same<T, void>::value);
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
struct return_self_fn {
    auto
    operator()(T2 n) {
        return impl(boost::mp11::mp_bool<!std::is_same<T, void>::value>{}, n);
    };

    auto
    impl(std::true_type, T2 n) {
        return n;
    };

    auto impl(std::false_type, T2){};
};

template <class T, class T2 = T>
void
set_package_task_tests(T2 test_value) {
    using namespace futures;
    FUTURES_STATIC_ASSERT(
        std::is_same<T, T2>::value || std::is_same<T, void>::value);


    auto return_self = return_self_fn<T, T2>{};

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

TEST_CASE("Shared state") {
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
                std::thread t([&p]() { p.set_value(2); });
                REQUIRE(f.get() == 2);
                t.join();
            }
            SECTION("Executor") {
                futures::asio::thread_pool t(1);
                futures::detail::execute(t, [&p]() { p.set_value(2); });
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
                futures::asio::thread_pool t(1);
                futures::asio::post(t, std::move(p));
                REQUIRE(f.get() == 2);
            }
        }
    }
}