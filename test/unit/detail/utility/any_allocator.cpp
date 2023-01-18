#include <futures/detail/utility/any_allocator.hpp>
//
#include <futures/detail/deps/boost/core/allocator_access.hpp>
#include <catch2/catch.hpp>

#ifdef __cpp_lib_memory_resource
#    include <memory_resource>
#endif


namespace {
    struct X {
        bool a;
        int b;
        char c;
        double d;

        template <class T>
        X(T const& v) {
            a = static_cast<bool>(v);
            b = static_cast<int>(v);
            c = static_cast<char>(v);
            d = static_cast<double>(v);
        }

        template <class T>
        X&
        operator=(T const& v) {
            a = static_cast<bool>(v);
            b = static_cast<int>(v);
            c = static_cast<char>(v);
            d = static_cast<double>(v);
            return *this;
        }

        friend bool
        operator==(X const& x, X const& y) {
            return x.a == y.a && x.b == y.b && x.c == y.c && x.d == y.d;
        }

        template <class T>
        friend bool
        operator==(X const& x, T const& t) {
            return x == X(t);
        }
    };
} // namespace

TEST_CASE("detail/utility/any_allocator") {
    using namespace futures::detail;

    using byte = unsigned char;
    using any_allocator_t = any_allocator<byte>;

    SECTION("any_allocator()") {
        STATIC_REQUIRE(std::is_default_constructible<any_allocator_t>::value);
        any_allocator_t a;
    }

    SECTION("any_allocator(Allocator const&)") {
        STATIC_REQUIRE(
            std::is_constructible<any_allocator_t, std::allocator<byte>>::value);
        any_allocator_t a(std::allocator<byte>{});
        any_allocator_t b(std::allocator<int>{});
    }

    SECTION("any_allocator(any_allocator<U> const&)") {
        any_allocator<byte> a(std::allocator<byte>{});
        any_allocator<int> b(a);
    }

    SECTION("any_allocator(any_allocator_t const&)") {
        STATIC_REQUIRE(std::is_copy_constructible<any_allocator_t>::value);
        any_allocator_t a(std::allocator<byte>{});
        any_allocator_t b(a);
    }

    SECTION("allocate / deallocate") {
        SECTION("byte") {
            SECTION("member function") {
                any_allocator_t a;
                byte* p = a.allocate(1, nullptr);
                *p = 'x';
                REQUIRE(*p == 'x');
                a.deallocate(p, 1);
            }
            SECTION("allocator access") {
                any_allocator_t a;
                byte* p = boost::allocator_allocate(a, 1, nullptr);
                *p = 'x';
                REQUIRE(*p == 'x');
                a.deallocate(p, 1);
            }
        }
        SECTION("X") {
            SECTION("member function") {
                any_allocator<X> a;
                X* p = a.allocate(1, nullptr);
                *p = 'x';
                REQUIRE(*p == 'x');
                a.deallocate(p, 1);
            }
            SECTION("allocator access") {
                any_allocator<X> a;
                X* p = boost::allocator_allocate(a, 1, nullptr);
                *p = 'x';
                REQUIRE(*p == 'x');
                a.deallocate(p, 1);
            }
        }
        SECTION("byte[]") {
            SECTION("member function") {
                any_allocator_t a;
                byte* p = a.allocate(10);
                p[0] = 'x';
                p[1] = 'y';
                p[2] = 'z';
                REQUIRE(p[0] == 'x');
                REQUIRE(p[1] == 'y');
                REQUIRE(p[2] == 'z');
                a.deallocate(p, 10);
            }
            SECTION("allocator access") {
                any_allocator_t a;
                byte* p = boost::allocator_allocate(a, 10);
                p[0] = 'x';
                p[1] = 'y';
                p[2] = 'z';
                REQUIRE(p[0] == 'x');
                REQUIRE(p[1] == 'y');
                REQUIRE(p[2] == 'z');
                a.deallocate(p, 10);
            }
        }
    }

    SECTION("construct / destroy") {
        SECTION("X") {
            SECTION("member function") {
                any_allocator<X> a;
                X* p = a.allocate(1, nullptr);
                a.construct(p, 'x');
                REQUIRE(*p == 'x');
                a.destroy(p);
                a.deallocate(p, 1);
            }
            SECTION("allocator access") {
                any_allocator<X> a;
                X* p = boost::allocator_allocate(a, 1, nullptr);
                boost::allocator_construct(a, p, 'x');
                REQUIRE(*p == 'x');
                boost::allocator_destroy(a, p);
                boost::allocator_deallocate(a, p, 1);
            }
        }
    }

    SECTION("allocate_bytes / deallocate_bytes") {
        SECTION("byte") {
            any_allocator_t a;
            byte* p = reinterpret_cast<byte*>(a.allocate_bytes(1, 1));
            *p = 'x';
            REQUIRE(*p == 'x');
            a.deallocate_bytes(p, 1, 1);
        }
        SECTION("byte[]") {
            any_allocator_t a;
            byte* p = reinterpret_cast<byte*>(a.allocate_bytes(10, 1));
            p[0] = 'x';
            p[1] = 'y';
            p[2] = 'z';
            REQUIRE(p[0] == 'x');
            REQUIRE(p[1] == 'y');
            REQUIRE(p[2] == 'z');
            a.deallocate_bytes(p, 10, 1);
        }
    }

    SECTION("allocate_object / deallocate_object") {
        any_allocator_t a;
        X* p = a.allocate_object<X>(1);
        a.construct(p, 'x');
        REQUIRE(*p == 'x');
        a.destroy(p);
        a.deallocate_object(p, 1);
    }

    SECTION("new_object / delete_object") {
        any_allocator_t a;
        X* p = a.new_object<X>('x');
        REQUIRE(*p == 'x');
        a.delete_object(p);
    }

    SECTION("select_on_container_copy_construction") {
        SECTION("std::allocator") {
            SECTION("member function") {
                any_allocator_t a;
                any_allocator_t b = a.select_on_container_copy_construction();
                // will not propagate on copy_construction but all
                // std::allocators compare equal
                REQUIRE(a == b);
                REQUIRE_FALSE(a != b);
            }
            SECTION("allocator access") {
                any_allocator_t a;
                any_allocator_t b = boost::
                    allocator_select_on_container_copy_construction(a);
                // will not propagate on copy_construction but all
                // std::allocators compare equal
                REQUIRE(a == b);
                REQUIRE_FALSE(a != b);
            }
        }

#ifdef __cpp_lib_memory_resource
        SECTION("monotonic_buffer_resource") {
            std::pmr::monotonic_buffer_resource buf(200);
            std::pmr::polymorphic_allocator<byte> pma(&buf);
            any_allocator_t a(pma);
            any_allocator_t b = a.select_on_container_copy_construction();
            // will not propagate on copy_construction and b will have an empty
            // buffer
            REQUIRE(a != b);
            REQUIRE_FALSE(a == b);
        }
#endif
    }
}
