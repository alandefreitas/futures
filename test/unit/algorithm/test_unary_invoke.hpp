//
// Copyright (c) 2022 alandefreitas (alandefreitas@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//

#ifndef FUTURES_TEST_UNARY_INVOKE_HPP
#define FUTURES_TEST_UNARY_INVOKE_HPP

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


#endif // FUTURES_TEST_UNARY_INVOKE_HPP
