#include <futures/algorithm.hpp>
//
#include <array>
#include <catch2/catch.hpp>

template <class Fn, class R, class Cb, class CheckCb>
void
test_void_unary_invoke(Fn fn, R&& r, Cb&& cb, CheckCb&& check) {
    SECTION("Basic") {
        SECTION("Ranges") {
            fn(r, cb);
            check();
        }

        SECTION("Iterators") {
            fn(r.begin(), r.end(), cb);
            check();
        }
    }

    auto ex = futures::make_default_executor();
    SECTION("Custom executor") {
        SECTION("Ranges") {
            fn(ex, r, cb);
            check();
        }

        SECTION("Iterators") {
            fn(ex, r.begin(), r.end(), cb);
            check();
        }
    }

    SECTION("Policy") {
        SECTION("Ranges") {
            fn(futures::seq, r, cb);
            check();
        }

        SECTION("Iterators") {
            fn(futures::seq, r.begin(), r.end(), cb);
            check();
        }
    }

    auto p =
        [](std::vector<int>::iterator first, std::vector<int>::iterator last) {
        return std::next(first, (last - first) / 2);
    };
    SECTION("Custom partitioner") {
        SECTION("Ranges") {
            fn(p, r, cb);
            check();
        }

        SECTION("Iterators") {
            fn(p, r.begin(), r.end(), cb);
            check();
        }
    }

    SECTION("Custom executor and partitioner") {
        SECTION("Ranges") {
            fn(ex, p, r, cb);
            check();
        }

        SECTION("Iterators") {
            fn(ex, p, r.begin(), r.end(), cb);
            check();
        }
    }
}

template <class Fn, class R, class Cb, class Result>
void
test_unary_invoke(Fn fn, R&& r, Cb&& cb, Result const& exp) {
    SECTION("Basic") {
        SECTION("Ranges") {
            auto res = fn(r, cb);
            REQUIRE(res == exp);
        }

        SECTION("Iterators") {
            auto res = fn(r.begin(), r.end(), cb);
            REQUIRE(res == exp);
        }
    }

    auto ex = futures::make_default_executor();
    SECTION("Custom executor") {
        SECTION("Ranges") {
            auto res = fn(ex, r, cb);
            REQUIRE(res == exp);
        }

        SECTION("Iterators") {
            auto res = fn(ex, r.begin(), r.end(), cb);
            REQUIRE(res == exp);
        }
    }

    SECTION("Policy") {
        SECTION("Ranges") {
            auto res = fn(futures::seq, r, cb);
            REQUIRE(res == exp);
        }

        SECTION("Iterators") {
            auto res = fn(futures::seq, r.begin(), r.end(), cb);
            REQUIRE(res == exp);
        }
    }

    auto p =
        [](std::vector<int>::iterator first, std::vector<int>::iterator last) {
        return std::next(first, (last - first) / 2);
    };
    SECTION("Custom partitioner") {
        SECTION("Ranges") {
            auto res = fn(p, r, cb);
            REQUIRE(res == exp);
        }

        SECTION("Iterators") {
            auto res = fn(p, r.begin(), r.end(), cb);
            REQUIRE(res == exp);
        }
    }

    SECTION("Custom executor and partitioner") {
        SECTION("Ranges") {
            auto res = fn(ex, p, r, cb);
            REQUIRE(res == exp);
        }

        SECTION("Iterators") {
            auto res = fn(ex, p, r.begin(), r.end(), cb);
            REQUIRE(res == exp);
        }
    }
}

template <class Fn, class R, class Cb, class Result>
void
test_binary_invoke(Fn fn, R&& r, Cb&& cb, Result const& exp) {
    SECTION("Basic") {
        SECTION("Ranges") {
            auto res = fn(r, cb);
            REQUIRE(res == exp);
        }

        SECTION("Iterators") {
            auto res = fn(r.begin(), r.end(), cb);
            REQUIRE(res == exp);
        }
    }

    auto ex = futures::make_default_executor();
    SECTION("Custom executor") {
        SECTION("Ranges") {
            auto res = fn(ex, r, cb);
            REQUIRE(res == exp);
        }

        SECTION("Iterators") {
            auto res = fn(ex, r.begin(), r.end(), cb);
            REQUIRE(res == exp);
        }
    }

    SECTION("Policy") {
        SECTION("Ranges") {
            auto res = fn(futures::seq, r, cb);
            REQUIRE(res == exp);
        }

        SECTION("Iterators") {
            auto res = fn(futures::seq, r.begin(), r.end(), cb);
            REQUIRE(res == exp);
        }
    }

    auto p =
        [](std::vector<int>::iterator first, std::vector<int>::iterator last) {
        return std::next(first, (last - first) / 2);
    };
    SECTION("Custom partitioner") {
        SECTION("Ranges") {
            auto res = fn(p, r, cb);
            REQUIRE(res == exp);
        }

        SECTION("Iterators") {
            auto res = fn(p, r.begin(), r.end(), cb);
            REQUIRE(res == exp);
        }
    }

    SECTION("Custom executor and partitioner") {
        SECTION("Ranges") {
            auto res = fn(ex, p, r, cb);
            REQUIRE(res == exp);
        }

        SECTION("Iterators") {
            auto res = fn(ex, p, r.begin(), r.end(), cb);
            REQUIRE(res == exp);
        }
    }
}

template <class Fn, class R, class T, class Result>
void
test_value_cmp(Fn fn, R&& r, T&& t, Result const& exp) {
    SECTION("Basic") {
        SECTION("Ranges") {
            auto res = fn(r, t);
            REQUIRE(res == exp);
        }

        SECTION("Iterators") {
            auto res = fn(r.begin(), r.end(), t);
            REQUIRE(res == exp);
        }
    }

    auto ex = futures::make_default_executor();
    SECTION("Custom executor") {
        SECTION("Ranges") {
            auto res = fn(ex, r, t);
            REQUIRE(res == exp);
        }

        SECTION("Iterators") {
            auto res = fn(ex, r.begin(), r.end(), t);
            REQUIRE(res == exp);
        }
    }

    SECTION("Policy") {
        SECTION("Ranges") {
            auto res = fn(futures::seq, r, t);
            REQUIRE(res == exp);
        }

        SECTION("Iterators") {
            auto res = fn(futures::seq, r.begin(), r.end(), t);
            REQUIRE(res == exp);
        }
    }

    auto p =
        [](std::vector<int>::iterator first, std::vector<int>::iterator last) {
        return std::next(first, (last - first) / 2);
    };
    SECTION("Custom partitioner") {
        SECTION("Ranges") {
            auto res = fn(p, r, t);
            REQUIRE(res == exp);
        }

        SECTION("Iterators") {
            auto res = fn(p, r.begin(), r.end(), t);
            REQUIRE(res == exp);
        }
    }

    SECTION("Custom executor and partitioner") {
        SECTION("Ranges") {
            auto res = fn(ex, p, r, t);
            REQUIRE(res == exp);
        }

        SECTION("Iterators") {
            auto res = fn(ex, p, r.begin(), r.end(), t);
            REQUIRE(res == exp);
        }
    }
}

TEST_CASE("Async algorithm") {
    using namespace futures;

    SECTION("for_each") {
        // range
        std::vector<int> v(5000);
        std::iota(v.begin(), v.end(), 1);

        // callback
        std::atomic<int> c(0);
        auto fun = [&c](int v) {
            c.fetch_add(v, std::memory_order_release);
        };

        // check
        int const v_sum = std::accumulate(v.begin(), v.end(), 0, std::plus<>());
        auto check = [&c, v_sum] {
            int r = c.load(std::memory_order_acquire);
            REQUIRE(r == v_sum);
            c.store(0, std::memory_order_release);
        };

        test_void_unary_invoke(for_each, v, fun, check);
    }

    SECTION("all_of") {
        std::vector<int> v(5000);
        std::iota(v.begin(), v.end(), 1);
        auto fun = [](int v) {
            return v < 5500;
        };
        test_unary_invoke(all_of, v, fun, true);
    }

    SECTION("any_of") {
        std::vector<int> v(5000);
        std::iota(v.begin(), v.end(), 1);
        auto fun = [](int v) {
            return v == 2700;
        };
        test_unary_invoke(any_of, v, fun, true);
    }

    SECTION("none_of") {
        std::vector<int> v(5000);
        std::iota(v.begin(), v.end(), 1);
        auto fun = [](int v) {
            return v > 5500;
        };
        test_unary_invoke(none_of, v, fun, true);
    }

    SECTION("find") {
        std::vector<int> v(5000);
        std::iota(v.begin(), v.end(), 1);
        test_value_cmp(find, v, 2700, v.begin() + 2699);
    }

    SECTION("find_if") {
        std::vector<int> v(5000);
        std::iota(v.begin(), v.end(), 1);
        auto fun = [](int v) {
            return v >= 2700;
        };
        test_unary_invoke(find_if, v, fun, v.begin() + 2699);
    }

    SECTION("find_if_not") {
        std::vector<int> v(5000);
        std::iota(v.begin(), v.end(), 1);
        auto fun = [](int v) {
            return v < 2700;
        };
        test_unary_invoke(find_if_not, v, fun, v.begin() + 2699);
    }

    SECTION("count") {
        std::vector<int> v(5000);
        std::iota(v.begin(), v.end(), 1);
        test_value_cmp(count, v, 2000, 1);
    }

    SECTION("count_if") {
        std::vector<int> v(5000);
        std::iota(v.begin(), v.end(), 1);
        auto fun = [](int v) {
            return v & 1;
        };
        test_unary_invoke(count_if, v, fun, 2500);
    }

    SECTION("reduce") {
        std::vector<int> v(5000);
        std::iota(v.begin(), v.end(), 1);
        auto custom_plus = [](int a, int b) {
            return a + b;
        };
        int const v_sum = std::accumulate(v.begin(), v.end(), 0, std::plus<>());
        test_binary_invoke(reduce, v, custom_plus, v_sum);
    }
}

#ifndef FUTURES_IS_SINGLE_HEADER
#    include <futures/detail/utility/is_constant_evaluated.hpp>
#    ifdef FUTURES_HAS_CONSTANT_EVALUATED
constexpr int
my_function() {
    if (futures::detail::is_constant_evaluated()) {
        return 1;
    } else {
        return 0;
    }
}

TEST_CASE("Is constant evaluated") {
    constexpr int v = my_function();
    INFO(v);
    REQUIRE(v == 1);
}

TEST_CASE("constexpr algorithms") {
    constexpr std::array<int, 5> a = { 1, 2, 3, 4, 5 };

    constexpr auto is_odd = [](int x) {
        return x & 1;
    };

    SECTION("all_of") {
        STATIC_REQUIRE(futures::all_of(a, is_odd) == false);
        STATIC_REQUIRE(futures::all_of(a.begin(), a.end(), is_odd) == false);
        futures::asio::thread_pool pool;
        auto ex = pool.executor();
        STATIC_REQUIRE(futures::all_of(ex, a, is_odd) == false);
        STATIC_REQUIRE(
            futures::all_of(ex, a.begin(), a.end(), is_odd) == false);
        constexpr auto p = futures::halve_partitioner(1);
        STATIC_REQUIRE(futures::all_of(p, a, is_odd) == false);
        STATIC_REQUIRE(futures::all_of(p, a.begin(), a.end(), is_odd) == false);
        STATIC_REQUIRE(futures::all_of(ex, p, a, is_odd) == false);
        STATIC_REQUIRE(
            futures::all_of(ex, p, a.begin(), a.end(), is_odd) == false);
    }

    SECTION("any_of") {
        STATIC_REQUIRE(futures::any_of(a, is_odd) == true);
        STATIC_REQUIRE(futures::any_of(a.begin(), a.end(), is_odd) == true);
        futures::asio::thread_pool pool;
        auto ex = pool.executor();
        STATIC_REQUIRE(futures::any_of(ex, a, is_odd) == true);
        STATIC_REQUIRE(futures::any_of(ex, a.begin(), a.end(), is_odd) == true);
        constexpr auto p = futures::halve_partitioner(1);
        STATIC_REQUIRE(futures::any_of(p, a, is_odd) == true);
        STATIC_REQUIRE(futures::any_of(p, a.begin(), a.end(), is_odd) == true);
        STATIC_REQUIRE(futures::any_of(ex, p, a, is_odd) == true);
        STATIC_REQUIRE(
            futures::any_of(ex, p, a.begin(), a.end(), is_odd) == true);
    }

    SECTION("none_of") {
        STATIC_REQUIRE(futures::none_of(a, is_odd) == false);
        STATIC_REQUIRE(futures::none_of(a.begin(), a.end(), is_odd) == false);
        futures::asio::thread_pool pool;
        auto ex = pool.executor();
        STATIC_REQUIRE(futures::none_of(ex, a, is_odd) == false);
        STATIC_REQUIRE(
            futures::none_of(ex, a.begin(), a.end(), is_odd) == false);
        constexpr auto p = futures::halve_partitioner(1);
        STATIC_REQUIRE(futures::none_of(p, a, is_odd) == false);
        STATIC_REQUIRE(
            futures::none_of(p, a.begin(), a.end(), is_odd) == false);
        STATIC_REQUIRE(futures::none_of(ex, p, a, is_odd) == false);
        STATIC_REQUIRE(
            futures::none_of(ex, p, a.begin(), a.end(), is_odd) == false);
    }

    SECTION("find") {}

    SECTION("find_if") {
        STATIC_REQUIRE(futures::find_if(a, is_odd) == a.begin());
        STATIC_REQUIRE(
            futures::find_if(a.begin(), a.end(), is_odd) == a.begin());
        futures::asio::thread_pool pool;
        auto ex = pool.executor();
        STATIC_REQUIRE(futures::find_if(ex, a, is_odd) == a.begin());
        STATIC_REQUIRE(
            futures::find_if(ex, a.begin(), a.end(), is_odd) == a.begin());
        constexpr auto p = futures::halve_partitioner(1);
        STATIC_REQUIRE(futures::find_if(p, a, is_odd) == a.begin());
        STATIC_REQUIRE(
            futures::find_if(p, a.begin(), a.end(), is_odd) == a.begin());
        STATIC_REQUIRE(futures::find_if(ex, p, a, is_odd) == a.begin());
        STATIC_REQUIRE(
            futures::find_if(ex, p, a.begin(), a.end(), is_odd) == a.begin());
    }

    SECTION("find_if_not") {
        STATIC_REQUIRE(futures::find_if_not(a, is_odd) == std::next(a.begin()));
        STATIC_REQUIRE(
            futures::find_if_not(a.begin(), a.end(), is_odd)
            == std::next(a.begin()));
        futures::asio::thread_pool pool;
        auto ex = pool.executor();
        STATIC_REQUIRE(
            futures::find_if_not(ex, a, is_odd) == std::next(a.begin()));
        STATIC_REQUIRE(
            futures::find_if_not(ex, a.begin(), a.end(), is_odd)
            == std::next(a.begin()));
        constexpr auto p = futures::halve_partitioner(1);
        STATIC_REQUIRE(
            futures::find_if_not(p, a, is_odd) == std::next(a.begin()));
        STATIC_REQUIRE(
            futures::find_if_not(p, a.begin(), a.end(), is_odd)
            == std::next(a.begin()));
        STATIC_REQUIRE(
            futures::find_if_not(ex, p, a, is_odd) == std::next(a.begin()));
        STATIC_REQUIRE(
            futures::find_if_not(ex, p, a.begin(), a.end(), is_odd)
            == std::next(a.begin()));
    }

    SECTION("count") {
        STATIC_REQUIRE(futures::count(a, 3) == 1);
        STATIC_REQUIRE(futures::count(a.begin(), a.end(), 3) == 1);
        futures::asio::thread_pool pool;
        auto ex = pool.executor();
        STATIC_REQUIRE(futures::count(ex, a, 3) == 1);
        STATIC_REQUIRE(futures::count(ex, a.begin(), a.end(), 3) == 1);
        constexpr auto p = futures::halve_partitioner(1);
        STATIC_REQUIRE(futures::count(p, a, 3) == 1);
        STATIC_REQUIRE(futures::count(p, a.begin(), a.end(), 3) == 1);
        STATIC_REQUIRE(futures::count(ex, p, a, 3) == 1);
        STATIC_REQUIRE(futures::count(ex, p, a.begin(), a.end(), 3) == 1);
    }

    SECTION("count_if") {
        STATIC_REQUIRE(futures::count_if(a, is_odd) == 3);
        STATIC_REQUIRE(futures::count_if(a.begin(), a.end(), is_odd) == 3);
        futures::asio::thread_pool pool;
        auto ex = pool.executor();
        STATIC_REQUIRE(futures::count_if(ex, a, is_odd) == 3);
        STATIC_REQUIRE(futures::count_if(ex, a.begin(), a.end(), is_odd) == 3);
        constexpr auto p = futures::halve_partitioner(1);
        STATIC_REQUIRE(futures::count_if(p, a, is_odd) == 3);
        STATIC_REQUIRE(futures::count_if(p, a.begin(), a.end(), is_odd) == 3);
        STATIC_REQUIRE(futures::count_if(ex, p, a, is_odd) == 3);
        STATIC_REQUIRE(
            futures::count_if(ex, p, a.begin(), a.end(), is_odd) == 3);
    }

    SECTION("reduce") {
        STATIC_REQUIRE(futures::reduce(a) == 15);
        STATIC_REQUIRE(futures::reduce(a, 0) == 15);
        STATIC_REQUIRE(futures::reduce(a.begin(), a.end()) == 15);
        STATIC_REQUIRE(futures::reduce(a.begin(), a.end(), 0) == 15);

        futures::asio::thread_pool pool;
        auto ex = pool.executor();
        STATIC_REQUIRE(futures::reduce(ex, a) == 15);
        STATIC_REQUIRE(futures::reduce(ex, a, 0) == 15);
        STATIC_REQUIRE(futures::reduce(ex, a.begin(), a.end()) == 15);
        STATIC_REQUIRE(futures::reduce(ex, a.begin(), a.end(), 0) == 15);

        constexpr auto p = futures::halve_partitioner(1);
        STATIC_REQUIRE(futures::reduce(p, a) == 15);
        STATIC_REQUIRE(futures::reduce(p, a, 0) == 15);
        STATIC_REQUIRE(futures::reduce(p, a.begin(), a.end()) == 15);
        STATIC_REQUIRE(futures::reduce(p, a.begin(), a.end(), 0) == 15);

        STATIC_REQUIRE(futures::reduce(ex, p, a) == 15);
        STATIC_REQUIRE(futures::reduce(ex, p, a, 0) == 15);
        STATIC_REQUIRE(futures::reduce(ex, p, a.begin(), a.end()) == 15);
        STATIC_REQUIRE(futures::reduce(ex, p, a.begin(), a.end(), 0) == 15);
    }
}
#    endif
#endif
