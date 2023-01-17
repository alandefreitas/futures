#include <futures/detail/utility/move_only_function.hpp>
//
#include <catch2/catch.hpp>

TEST_CASE("move_only_function") {
    using namespace futures::detail;

    SECTION("move_only_function()") {
        move_only_function<void()> f;
        REQUIRE_FALSE(f);
    }
    SECTION("move_only_function(nullptr_t)") {
        move_only_function<void()> f(nullptr);
        REQUIRE_FALSE(f);
    }
    SECTION("move_only_function(F &&)") {
        move_only_function<int()> f([] { return 2; });
        REQUIRE(f);
        REQUIRE(f() == 2);
    }
    SECTION("move_only_function(move_only_function const &)") {
        STATIC_REQUIRE(!is_copy_constructible_v<move_only_function<void()>>);
    }
    SECTION("move_only_function(move_only_function &&)") {
        move_only_function<int()> a([] { return 2; });
        move_only_function<int()> b(std::move(a));
        REQUIRE_FALSE(a);
        REQUIRE(b);
        REQUIRE(b() == 2);
    }
    SECTION("move_only_function(in_place_type_t<T>, CArgs &&...)") {
        struct lambda {
            int a;
            lambda(int v) : a(v) {}
            int
            operator()() {
                return a * 2;
            }
        };
        move_only_function<int()> a(in_place_type_t<lambda>{}, 2);
        REQUIRE(a);
        REQUIRE(a() == 4);
    }
    SECTION("op=(move_only_function &&)") {
        move_only_function<int()> a([] { return 2; });
        REQUIRE(a);
        move_only_function<int()> b;
        REQUIRE_FALSE(b);
        b = std::move(a);
        REQUIRE_FALSE(a);
        REQUIRE(b);
        REQUIRE(b() == 2);
    }
    SECTION("op=(move_only_function const &)") {
        STATIC_REQUIRE(
            !std::is_copy_assignable<move_only_function<void()>>::value);
    }
    SECTION("op=(std::nullptr_t)") {
        move_only_function<int()> a([] { return 2; });
        REQUIRE(a);
        REQUIRE(a() == 2);
        a = nullptr;
        REQUIRE_FALSE(a);
    }
    SECTION("op=(F &&f)") {
        struct lambda {
            int a;
            bool empty = true;

            lambda(int v) : a(v), empty(false) {}

            lambda(lambda const& other) : a(other.a), empty(other.empty) {}

            lambda(lambda&& other) noexcept : a(other.a), empty(other.empty) {
                other.empty = true;
                other.a = 0;
            }

            int
            operator()() {
                return a * 2;
            }
        };

        SECTION("Copy") {
            move_only_function<int()> a;
            REQUIRE_FALSE(a);
            lambda b(2);
            REQUIRE(b() == 4);
            REQUIRE_FALSE(b.empty);
            a = b;
            REQUIRE(b() == 4);
            REQUIRE_FALSE(b.empty);
            REQUIRE(a);
            REQUIRE(a() == 4);
        }

        SECTION("Move") {
            move_only_function<int()> a;
            REQUIRE_FALSE(a);
            lambda b(2);
            REQUIRE(b() == 4);
            REQUIRE_FALSE(b.empty);
            a = std::move(b);
            REQUIRE(b() == 0);
            REQUIRE(b.empty);
            REQUIRE(a);
            REQUIRE(a() == 4);
        }
    }
    SECTION("swap(move_only_function &)") {
        move_only_function<int()> a([] { return 2; });
        move_only_function<int()> b([] { return 3; });
        std::swap(a, b);
        REQUIRE(a() == 3);
        REQUIRE(b() == 2);
    }
    SECTION("op bool()") {
        move_only_function<int()> a([] { return 2; });
        REQUIRE(a);
        a = nullptr;
        REQUIRE_FALSE(a);
    }
    SECTION("op ==()") {
        struct lambda {
            int a;
            bool empty = true;

            lambda(int v) : a(v), empty(false) {}

            lambda(lambda const& other) : a(other.a), empty(other.empty) {}

            lambda(lambda&& other) noexcept : a(other.a), empty(other.empty) {
                other.empty = true;
                other.a = 0;
            }

            int
            operator()() const {
                return a * 2;
            }
        };

        SECTION("value") {
            move_only_function<int()> a(lambda(2));
            REQUIRE(a() == 4);
            REQUIRE(a() == 4);
        }

        SECTION("lvalue") {
            move_only_function<int()&> a(lambda(2));
            REQUIRE(a() == 4);
            REQUIRE(a() == 4);
        }

        SECTION("rvalue") {
            move_only_function<int() &&> a(lambda(2));
            REQUIRE(std::move(a)() == 4);
            REQUIRE(std::move(a)() == 4);
        }

        SECTION("value") {
            move_only_function<int() const> a(lambda(2));
            REQUIRE(a() == 4);
            REQUIRE(a() == 4);
        }

        SECTION("lvalue") {
            move_only_function<int() const&> a(lambda(2));
            REQUIRE(a() == 4);
            REQUIRE(a() == 4);
        }

        SECTION("rvalue") {
            move_only_function<int() const&&> a(lambda(2));
            REQUIRE(std::move(a)() == 4);
            REQUIRE(std::move(a)() == 4);
        }
    }
}
