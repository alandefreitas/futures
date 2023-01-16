#include <futures/detail/utility/sbo_ptr.hpp>
//
#include <catch2/catch.hpp>

struct I {
    int
    v() const {
        return 10;
    }
};

struct X : public I {
    unsigned char sbo_buffer_[16];
};

TEST_CASE("sbo_ptr") {
    using namespace futures::detail;

    SECTION("sbo_ptr") {
        SECTION("sbo_ptr()") {
            sbo_ptr<int> p;
            REQUIRE_FALSE(p);
        }
        SECTION("sbo_ptr(Allocator const&)") {
            SECTION("small") {
                std::allocator<int> a;
                sbo_ptr<int> p(a);
                REQUIRE_FALSE(p);
                p.emplace<int>(3);
                REQUIRE(p);
            }
            SECTION("large") {
                std::allocator<I> a;
                sbo_ptr<I> p(a);
                REQUIRE_FALSE(p);
                p.emplace<X>();
                REQUIRE(p);
            }
        }
        SECTION("sbo_ptr(sbo_ptr_base const&)") {
            SECTION("small") {
                sbo_ptr<int> a;
                REQUIRE_FALSE(a);
                a = 3;
                REQUIRE(a);
                REQUIRE(*a == 3);
                sbo_ptr<int> b(a);
                REQUIRE(b);
                REQUIRE(*b == 3);
                REQUIRE(a);
                REQUIRE(*a == 3);
            }
            SECTION("large") {
                sbo_ptr<I> a;
                REQUIRE_FALSE(a);
                a = X{};
                REQUIRE(a);
                sbo_ptr<I> b(a);
                REQUIRE(b);
                REQUIRE(a);
            }
        }
        SECTION("sbo_ptr(sbo_ptr_base&&)") {
            SECTION("small") {
                sbo_ptr<int> a;
                REQUIRE_FALSE(a);
                a = 3;
                REQUIRE(a);
                REQUIRE(*a == 3);
                sbo_ptr<int> b(std::move(a));
                REQUIRE(b);
                REQUIRE(*b == 3);
                REQUIRE_FALSE(a);
            }
            SECTION("Large") {
                sbo_ptr<I> a;
                REQUIRE_FALSE(a);
                a = X{};
                REQUIRE(a);
                sbo_ptr<I> b(std::move(a));
                REQUIRE(b);
                REQUIRE_FALSE(a);
            }
        }
        SECTION("sbo_ptr(in_place_type_t<U>, Args&&...)") {
            SECTION("small") {
                sbo_ptr<int> a(in_place_type_t<int>{}, 3);
                REQUIRE(a);
                REQUIRE(*a == 3);
            }
            SECTION("Large") {
                sbo_ptr<I> a(in_place_type_t<X>{});
                REQUIRE(a);
            }
        }
        SECTION("sbo_ptr(U&&)") {
            int v = 3;
            sbo_ptr<int> a(std::move(v));
            REQUIRE(a);
            REQUIRE(*a == 3);
        }
        SECTION("op=(U&&)") {
            sbo_ptr<int> a;
            REQUIRE_FALSE(a);
            int v = 3;
            a = std::move(v);
            REQUIRE(a);
            REQUIRE(*a == 3);
        }
        SECTION("op=(U const&)") {
            sbo_ptr<int> a;
            REQUIRE_FALSE(a);
            int v = 3;
            a = v;
            REQUIRE(a);
            REQUIRE(*a == 3);
        }
        SECTION("op=(nullptr_t)") {
            sbo_ptr<int> a(3);
            REQUIRE(a);
            a = nullptr;
            REQUIRE_FALSE(a);
        }
        SECTION("Pointer ops") {
            sbo_ptr<int> a(3);
            REQUIRE(a.get());
            REQUIRE(::futures::detail::as_const(a).get());
            REQUIRE(*a == 3);
            REQUIRE(*::futures::detail::as_const(a) == 3);

            sbo_ptr<I> x(X{});
            REQUIRE(x->v() == 10);
            REQUIRE(::futures::detail::as_const(x)->v() == 10);

            sbo_ptr<int> b(a);
            REQUIRE_FALSE(a == b);
            REQUIRE_FALSE(a == nullptr);
            REQUIRE_FALSE(nullptr == a);
            REQUIRE(a != b);
            REQUIRE(a != nullptr);
            REQUIRE(nullptr != a);

            a.reset();
            REQUIRE_FALSE(a.get());
            REQUIRE_FALSE(::futures::detail::as_const(a).get());
            REQUIRE_FALSE(a == b);
            REQUIRE(a == nullptr);
            REQUIRE(nullptr == a);
            REQUIRE(a != b);
            REQUIRE_FALSE(a != nullptr);
            REQUIRE_FALSE(nullptr != a);
        }
    }

    SECTION("static_sbo_ptr") {
        SECTION("static_sbo_ptr()") {
            static_sbo_ptr<int> p;
            REQUIRE_FALSE(p);
        }
        SECTION("static_sbo_ptr(Allocator const&)") {
            SECTION("small") {
                std::allocator<int> a;
                static_sbo_ptr<int> p(a);
                REQUIRE_FALSE(p);
                p.emplace<int>(3);
                REQUIRE(p);
            }
            SECTION("large") {
                std::allocator<I> a;
                static_sbo_ptr<I> p(a);
                REQUIRE_FALSE(p);
                p.emplace<X>();
                REQUIRE(p);
            }
        }
        SECTION("static_sbo_ptr(static_sbo_ptr_base const&)") {
            SECTION("small") {
                static_sbo_ptr<int> a;
                REQUIRE_FALSE(a);
                a = 3;
                REQUIRE(a);
                REQUIRE(*a == 3);
                static_sbo_ptr<int> b(*a);
                REQUIRE(b);
                REQUIRE(*b == 3);
                REQUIRE(a);
                REQUIRE(*a == 3);
            }
            SECTION("large") {
                static_sbo_ptr<I> a;
                REQUIRE_FALSE(a);
                a = X{};
                REQUIRE(a);
                static_sbo_ptr<I> b(*a);
                REQUIRE(b);
                REQUIRE(a);
            }
        }
        SECTION("static_sbo_ptr(in_place_type_t<U>, Args&&...)") {
            SECTION("small") {
                static_sbo_ptr<int> a(in_place_type_t<int>{}, 3);
                REQUIRE(a);
                REQUIRE(*a == 3);
            }
            SECTION("Large") {
                static_sbo_ptr<I> a(in_place_type_t<X>{});
                REQUIRE(a);
            }
        }
        SECTION("static_sbo_ptr(U&&)") {
            int v = 3;
            static_sbo_ptr<int> a(std::move(v));
            REQUIRE(a);
            REQUIRE(*a == 3);
        }
        SECTION("op=(U&&)") {
            static_sbo_ptr<int> a;
            REQUIRE_FALSE(a);
            int v = 3;
            a = std::move(v);
            REQUIRE(a);
            REQUIRE(*a == 3);
        }
        SECTION("op=(U const&)") {
            static_sbo_ptr<int> a;
            REQUIRE_FALSE(a);
            int v = 3;
            a = v;
            REQUIRE(a);
            REQUIRE(*a == 3);
        }
        SECTION("op=(nullptr_t)") {
            static_sbo_ptr<int> a(3);
            REQUIRE(a);
            a = nullptr;
            REQUIRE_FALSE(a);
        }
        SECTION("Pointer ops") {
            static_sbo_ptr<int> a(3);
            REQUIRE(a.get());
            REQUIRE(::futures::detail::as_const(a).get());
            REQUIRE(*a == 3);
            REQUIRE(*::futures::detail::as_const(a) == 3);

            static_sbo_ptr<I> x(X{});
            REQUIRE(x->v() == 10);
            REQUIRE(::futures::detail::as_const(x)->v() == 10);

            static_sbo_ptr<int> b(*a);
            REQUIRE_FALSE(a == b);
            REQUIRE_FALSE(a == nullptr);
            REQUIRE_FALSE(nullptr == a);
            REQUIRE(a != b);
            REQUIRE(a != nullptr);
            REQUIRE(nullptr != a);

            a.reset();
            REQUIRE_FALSE(a.get());
            REQUIRE_FALSE(::futures::detail::as_const(a).get());
            REQUIRE_FALSE(a == b);
            REQUIRE(a == nullptr);
            REQUIRE(nullptr == a);
            REQUIRE(a != b);
            REQUIRE_FALSE(a != nullptr);
            REQUIRE_FALSE(nullptr != a);
        }
    }

    SECTION("move_only_sbo_ptr") {
        SECTION("move_only_sbo_ptr()") {
            move_only_sbo_ptr<int> p;
            REQUIRE_FALSE(p);
        }
        SECTION("move_only_sbo_ptr(Allocator const&)") {
            SECTION("small") {
                std::allocator<int> a;
                move_only_sbo_ptr<int> p(a);
                REQUIRE_FALSE(p);
                p.emplace<int>(3);
                REQUIRE(p);
            }
            SECTION("large") {
                std::allocator<I> a;
                move_only_sbo_ptr<I> p(a);
                REQUIRE_FALSE(p);
                p.emplace<X>();
                REQUIRE(p);
            }
        }
        SECTION("move_only_sbo_ptr(move_only_sbo_ptr_base&&)") {
            SECTION("small") {
                move_only_sbo_ptr<int> a;
                REQUIRE_FALSE(a);
                a = 3;
                REQUIRE(a);
                REQUIRE(*a == 3);
                move_only_sbo_ptr<int> b(std::move(a));
                REQUIRE(b);
                REQUIRE(*b == 3);
                REQUIRE_FALSE(a);
            }
            SECTION("Large") {
                move_only_sbo_ptr<I> a;
                REQUIRE_FALSE(a);
                a = X{};
                REQUIRE(a);
                move_only_sbo_ptr<I> b(std::move(a));
                REQUIRE(b);
                REQUIRE_FALSE(a);
            }
        }
        SECTION("move_only_sbo_ptr(in_place_type_t<U>, Args&&...)") {
            SECTION("small") {
                move_only_sbo_ptr<int> a(in_place_type_t<int>{}, 3);
                REQUIRE(a);
                REQUIRE(*a == 3);
            }
            SECTION("Large") {
                move_only_sbo_ptr<I> a(in_place_type_t<X>{});
                REQUIRE(a);
            }
        }
        SECTION("move_only_sbo_ptr(U&&)") {
            int v = 3;
            move_only_sbo_ptr<int> a(std::move(v));
            REQUIRE(a);
            REQUIRE(*a == 3);
        }
        SECTION("op=(U&&)") {
            move_only_sbo_ptr<int> a;
            REQUIRE_FALSE(a);
            int v = 3;
            a = std::move(v);
            REQUIRE(a);
            REQUIRE(*a == 3);
        }
        SECTION("op=(U const&)") {
            move_only_sbo_ptr<int> a;
            REQUIRE_FALSE(a);
            int v = 3;
            a = v;
            REQUIRE(a);
            REQUIRE(*a == 3);
        }
        SECTION("op=(nullptr_t)") {
            move_only_sbo_ptr<int> a(3);
            REQUIRE(a);
            a = nullptr;
            REQUIRE_FALSE(a);
        }
        SECTION("Pointer ops") {
            move_only_sbo_ptr<int> a(3);
            REQUIRE(a.get());
            REQUIRE(::futures::detail::as_const(a).get());
            REQUIRE(*a == 3);
            REQUIRE(*::futures::detail::as_const(a) == 3);

            move_only_sbo_ptr<I> x(X{});
            REQUIRE(x->v() == 10);
            REQUIRE(::futures::detail::as_const(x)->v() == 10);

            move_only_sbo_ptr<int> b(*a);
            REQUIRE_FALSE(a == b);
            REQUIRE_FALSE(a == nullptr);
            REQUIRE_FALSE(nullptr == a);
            REQUIRE(a != b);
            REQUIRE(a != nullptr);
            REQUIRE(nullptr != a);

            a.reset();
            REQUIRE_FALSE(a.get());
            REQUIRE_FALSE(::futures::detail::as_const(a).get());
            REQUIRE_FALSE(a == b);
            REQUIRE(a == nullptr);
            REQUIRE(nullptr == a);
            REQUIRE(a != b);
            REQUIRE_FALSE(a != nullptr);
            REQUIRE_FALSE(nullptr != a);
        }
    }
}
