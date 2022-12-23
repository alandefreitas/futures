#include <futures/detail/utility/regular_void.hpp>
//
#include <catch2/catch.hpp>

namespace futures {
    namespace detail {
        bool
        operator==(
            futures::detail::regular_void,
            futures::detail::regular_void) {
            return true;
        }
    } // namespace detail
} // namespace futures

TEST_CASE("Regular void") {
    using namespace futures::detail;

    SECTION("Conversions") {
        STATIC_REQUIRE(is_same_v<make_regular_t<void>, regular_void>);
        REQUIRE(make_irregular(2) == 2);
        auto fn = []() {
            return make_irregular(regular_void{});
        };
        STATIC_REQUIRE(is_same_v<invoke_result_t<decltype(fn)>, void>);
    }

    SECTION("Invoke") {
        SECTION("No params") {
            auto fn = []() {
                return 2;
            };
            STATIC_REQUIRE(
                is_same_v<regular_invoke_result_t<decltype(fn)>, int>);
            REQUIRE(regular_void_invoke(fn) == 2);
        }

        SECTION("Single regular_void") {
            auto fn = []() {
                return 2;
            };
            STATIC_REQUIRE(
                is_same_v<
                    regular_invoke_result_t<decltype(fn), regular_void>,
                    int>);
            REQUIRE(regular_void_invoke(fn, regular_void{}) == 2);
        }

        SECTION("Two regular_voids") {
            auto fn = []() {
                return 2;
            };
            STATIC_REQUIRE(
                is_same_v<
                    regular_invoke_result_t<
                        decltype(fn),
                        regular_void,
                        regular_void>,
                    int>);
            REQUIRE(
                regular_void_invoke(fn, regular_void{}, regular_void{}) == 2);
        }

        SECTION("Int and regular void") {
            auto fn = [](int x) {
                return 2 * x;
            };
            STATIC_REQUIRE(
                is_same_v<
                    regular_invoke_result_t<decltype(fn), int, regular_void>,
                    int>);
            REQUIRE(regular_void_invoke(fn, 2, regular_void{}) == 4);
        }

        SECTION("Regular void and int") {
            auto fn = [](int x) {
                return 2 * x;
            };
            STATIC_REQUIRE(
                is_same_v<
                    regular_invoke_result_t<decltype(fn), regular_void, int>,
                    int>);
            REQUIRE(regular_void_invoke(fn, regular_void{}, 2) == 4);
        }

        SECTION("Interleaved") {
            auto fn = [](int x) {
                return 2 * x;
            };
            STATIC_REQUIRE(
                is_same_v<
                    regular_invoke_result_t<
                        decltype(fn),
                        regular_void,
                        int,
                        regular_void>,
                    int>);
            REQUIRE(
                regular_void_invoke(fn, regular_void{}, 2, regular_void{})
                == 4);
        }

        SECTION("Return regular") {
            auto fn = [](int) {
            };
            STATIC_REQUIRE(
                is_same_v<
                    regular_invoke_result_t<
                        decltype(fn),
                        regular_void,
                        int,
                        regular_void>,
                    regular_void>);
            REQUIRE(
                regular_void_invoke(fn, regular_void{}, 2, regular_void{})
                == regular_void{});
        }
    }

    SECTION("Make tuple") {
        SECTION("No params") {
            REQUIRE(make_irregular_tuple() == std::make_tuple());
        }

        SECTION("Single regular_void") {
            REQUIRE(make_irregular_tuple(regular_void{}) == std::make_tuple());
        }

        SECTION("Two regular_voids") {
            REQUIRE(
                make_irregular_tuple(regular_void{}, regular_void{})
                == std::make_tuple());
        }

        SECTION("Int and regular void") {
            REQUIRE(
                make_irregular_tuple(2, regular_void{}) == std::make_tuple(2));
        }

        SECTION("Regular void and int") {
            REQUIRE(
                make_irregular_tuple(regular_void{}, 2) == std::make_tuple(2));
        }

        SECTION("Interleaved") {
            REQUIRE(
                make_irregular_tuple(regular_void{}, 2, regular_void{})
                == std::make_tuple(2));
        }
    }
}
