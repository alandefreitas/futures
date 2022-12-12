#include <futures/algorithm/traits/is_bidirectional_iterator.hpp>
//
#include <catch2/catch.hpp>
#include <vector>

TEST_CASE("algorithm traits is bidirectional iterator") {
    using namespace futures;
    {
        using I = std::vector<int>::iterator;
        STATIC_REQUIRE(is_input_iterator_v<I>);
        STATIC_REQUIRE(is_derived_from_v<detail::iter_concept_t<I>, std::forward_iterator_tag>);
        STATIC_REQUIRE(is_incrementable_v<I>);
        STATIC_REQUIRE(is_sentinel_for_v<I, I>);
        STATIC_REQUIRE(is_forward_iterator_v<I>);
        STATIC_REQUIRE(is_derived_from_v<detail::iter_concept_t<I>, std::bidirectional_iterator_tag>);
        STATIC_REQUIRE(std::is_same_v<decltype(--std::declval<I>()), I&>);
        STATIC_REQUIRE(std::is_same_v<decltype(std::declval<I>()--), I>);
        STATIC_REQUIRE(is_bidirectional_iterator_v<I>);
    }
    {
        using I = int*;
        STATIC_REQUIRE(is_input_iterator_v<I>);
        STATIC_REQUIRE(is_derived_from_v<detail::iter_concept_t<I>, std::forward_iterator_tag>);
        STATIC_REQUIRE(is_regular_v<I>);
        STATIC_REQUIRE(is_movable_v<I>);
        STATIC_REQUIRE(std::is_same_v<decltype(++std::declval<I&>()), I&>);
        STATIC_REQUIRE(std::is_same_v<iter_difference_t<I>, std::ptrdiff_t>);
        STATIC_REQUIRE(is_weakly_incrementable_v<I>);
        STATIC_REQUIRE(std::is_same_v<decltype(std::declval<I&>()++), I>);
        STATIC_REQUIRE(is_incrementable_v<I>);
        STATIC_REQUIRE(is_sentinel_for_v<I, I>);
        STATIC_REQUIRE(is_forward_iterator_v<I>);
        STATIC_REQUIRE(is_derived_from_v<detail::iter_concept_t<I>, std::bidirectional_iterator_tag>);
        STATIC_REQUIRE(std::is_same_v<decltype(--std::declval<I&>()), I&>);
        STATIC_REQUIRE(std::is_same_v<decltype(std::declval<I&>()--), I>);
        STATIC_REQUIRE(is_bidirectional_iterator_v<I>);
    }
}
