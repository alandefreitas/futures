#include <futures/detail/utility/is_constant_evaluated.hpp>
//
#include <catch2/catch.hpp>

FUTURES_CONSTANT_EVALUATED_CONSTEXPR
int
my_function() {
    if (futures::detail::is_constant_evaluated()) {
        return 1;
    } else {
        return 0;
    }
}

TEST_CASE("is constant evaluated") {
    FUTURES_CONSTANT_EVALUATED_CONSTEXPR
    int v = my_function();
    INFO(v);
#ifdef FUTURES_HAS_CONSTANT_EVALUATED
    REQUIRE(v == 1);
#else
    REQUIRE(v == 0);
#endif
}
