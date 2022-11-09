#include <futures/detail/utility/is_constant_evaluated.hpp>
//
#include <catch2/catch.hpp>

#ifdef FUTURES_HAS_CONSTANT_EVALUATED
constexpr int
my_function() {
    if (futures::detail::is_constant_evaluated()) {
        return 1;
    } else {
        return 0;
    }
}

TEST_CASE("is constant evaluated") {
    constexpr int v = my_function();
    INFO(v);
    REQUIRE(v == 1);
}
#endif
