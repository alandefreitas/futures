//
// Copyright (c) 2022 alandefreitas (alandefreitas@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//

#ifndef FUTURES_TEST_VALUE_CMP_HPP
#define FUTURES_TEST_VALUE_CMP_HPP

#include <catch2/catch.hpp>

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


#endif // FUTURES_TEST_VALUE_CMP_HPP
