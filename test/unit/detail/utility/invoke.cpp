#include <futures/detail/utility/invoke.hpp>
//
#include <catch2/catch.hpp>

int
times_two(int i) {
    return i * 2;
}

int
times_two_nt(int i) noexcept {
    return i * 2;
}

TEST_CASE("invoke") {
    using namespace futures::detail;

    SECTION("Function pointer") {
        SECTION("throw") {
            STATIC_REQUIRE(std::is_function<decltype(times_two)>::value);
            STATIC_REQUIRE(!std::is_member_pointer<decltype(times_two)>::value);
            REQUIRE(invoke(times_two, 1) == 2);
            STATIC_REQUIRE(is_invocable_v<decltype(times_two), int>);
            STATIC_REQUIRE(!is_nothrow_invocable_v<decltype(times_two), int>);
            STATIC_REQUIRE(
                std::is_same<invoke_result_t<decltype(times_two), int>, int>::
                    value);
            STATIC_REQUIRE(
                std::is_same<invoke_result_t<decltype(times_two), int>, int>::
                    value);
#ifdef __cpp_lib_is_invocable
            STATIC_REQUIRE(
                std::is_same<
                    std::invoke_result_t<decltype(times_two), int>,
                    int>::value);
            STATIC_REQUIRE(
                std::is_same<
                    std::invoke_result_t<decltype(times_two), int>,
                    int>::value);
#endif
        }

        SECTION("nothrow") {
            STATIC_REQUIRE(std::is_function<decltype(times_two_nt)>::value);
            STATIC_REQUIRE(
                !std::is_member_pointer<decltype(times_two_nt)>::value);
            REQUIRE(invoke(times_two_nt, 1) == 2);
            STATIC_REQUIRE(is_invocable_v<decltype(times_two_nt), int>);
            STATIC_REQUIRE(
                is_invocable_impl<
                    invoke_result_impl<decltype(times_two_nt), int>,
                    void>::value);
#ifdef __cpp_noexcept_function_type
            STATIC_REQUIRE(is_nothrow_invocable_v<decltype(times_two_nt), int>);
#endif
            STATIC_REQUIRE(
                std::is_same<
                    invoke_result_t<decltype(times_two_nt), int>,
                    int>::value);
            STATIC_REQUIRE(
                std::is_same<
                    invoke_result_t<decltype(times_two_nt), int>,
                    int>::value);
#ifdef __cpp_lib_is_invocable
            STATIC_REQUIRE(
                std::is_same<
                    std::invoke_result_t<decltype(times_two_nt), int>,
                    int>::value);
            STATIC_REQUIRE(
                std::is_same<
                    std::invoke_result_t<decltype(times_two_nt), int>,
                    int>::value);
#endif
        }
    }


    SECTION("Callable") {
        SECTION("Functor") {
            SECTION("No capture") {
                struct times_three_fn {
                    int
                    operator()(int i) const {
                        return i * 3;
                    }
                };
                STATIC_REQUIRE(!std::is_function<times_three_fn>::value);
                STATIC_REQUIRE(!std::is_member_pointer<
                               std::decay_t<times_three_fn>>::value);
                REQUIRE(invoke(times_three_fn{}, 1) == 3);
                STATIC_REQUIRE(is_invocable_v<times_three_fn, int>);
                STATIC_REQUIRE(
                    std::is_same<invoke_result_t<times_three_fn, int>, int>::
                        value);
                STATIC_REQUIRE(
                    std::is_same<invoke_result_t<times_three_fn, int>, int>::
                        value);
#ifdef __cpp_lib_is_invocable
                REQUIRE(std::invoke(times_three_fn{}, 1) == 3);
                STATIC_REQUIRE(std::is_invocable_v<times_three_fn, int>);
                STATIC_REQUIRE(
                    std::is_same<
                        std::invoke_result_t<times_three_fn, int>,
                        int>::value);
                STATIC_REQUIRE(
                    std::is_same<
                        std::invoke_result_t<times_three_fn, int>,
                        int>::value);
#endif
            }

            SECTION("Capture") {
                struct times_n_fn {
                    times_n_fn(int num) : num_(num) {}
                    int
                    operator()(int i) const {
                        return num_ * i;
                    }
                    int num_;
                };
                STATIC_REQUIRE(!std::is_function<times_n_fn>::value);
                STATIC_REQUIRE(
                    !std::is_member_pointer<std::decay_t<times_n_fn>>::value);
                REQUIRE(invoke(times_n_fn(4), 1) == 4);
                REQUIRE(invoke(times_n_fn(5), 1) == 5);
                STATIC_REQUIRE(is_invocable_v<times_n_fn, int>);
                STATIC_REQUIRE(
                    std::is_same<invoke_result_t<times_n_fn, int>, int>::value);
#ifdef __cpp_lib_is_invocable
                STATIC_REQUIRE(
                    std::is_same<std::invoke_result_t<times_n_fn, int>, int>::
                        value);
#endif
            }
        }

        SECTION("Lambda") {
            SECTION("No capture") {
                auto fn = [](int i) {
                    return i * 8;
                };
                STATIC_REQUIRE(!std::is_function<decltype(fn)>::value);
                STATIC_REQUIRE(!std::is_member_pointer<decltype(fn)>::value);
                REQUIRE(invoke(fn, 1) == 8);
                STATIC_REQUIRE(is_invocable_v<decltype(fn), int>);
                STATIC_REQUIRE(
                    std::is_same<invoke_result_t<decltype(fn), int>, int>::
                        value);
#ifdef __cpp_lib_is_invocable
                STATIC_REQUIRE(
                    std::is_same<std::invoke_result_t<decltype(fn), int>, int>::
                        value);
#endif
            }

            SECTION("Capture") {
                int m = 0;
                auto fn = [&m](int i) {
                    ++m;
                    return i * 8 * m;
                };
                STATIC_REQUIRE(!std::is_function<decltype(fn)>::value);
                STATIC_REQUIRE(!std::is_member_pointer<decltype(fn)>::value);
                REQUIRE(invoke(fn, 1) == 8);
                REQUIRE(invoke(fn, 1) == 16);
                STATIC_REQUIRE(is_invocable_v<decltype(fn), int>);
                STATIC_REQUIRE(
                    std::is_same<invoke_result_t<decltype(fn), int>, int>::
                        value);
#ifdef __cpp_lib_is_invocable
                STATIC_REQUIRE(std::is_invocable_v<decltype(fn), int>);
                STATIC_REQUIRE(
                    std::is_same<std::invoke_result_t<decltype(fn), int>, int>::
                        value);
#endif
            }
        }
    }

    SECTION("Member pointer") {
        struct operand {
            operand(int num) : num_(num) {}
            int
            mult(int i) const {
                return num_ * i;
            }
            int num_;
        };
        SECTION("Function pointer") {
            STATIC_REQUIRE(!std::is_function<decltype(&operand::mult)>::value);
            STATIC_REQUIRE(
                std::is_member_pointer<decltype(&operand::mult)>::value);
            SECTION("Self") {
                REQUIRE(invoke(&operand::mult, operand(6), 1) == 6);
                STATIC_REQUIRE(
                    is_invocable_v<decltype(&operand::mult), operand, int>);
                STATIC_REQUIRE(
                    is_same_v<
                        invoke_result_t<decltype(&operand::mult), operand, int>,
                        int>);
#ifdef __cpp_lib_is_invocable
                STATIC_REQUIRE(
                    is_same_v<
                        std::invoke_result_t<
                            decltype(&operand::mult),
                            operand,
                            int>,
                        int>);
#endif
            }
            SECTION("Ref wrapper") {
                operand o(7);
                std::reference_wrapper<operand> r = std::ref(o);
                REQUIRE(::futures::detail::invoke(&operand::mult, r, 1) == 7);
                STATIC_REQUIRE(
                    is_invocable_v<
                        decltype(&operand::mult),
                        std::reference_wrapper<operand>,
                        int>);
                STATIC_REQUIRE(
                    is_same_v<
                        invoke_result_t<
                            decltype(&operand::mult),
                            std::reference_wrapper<operand>,
                            int>,
                        int>);
#ifdef __cpp_lib_is_invocable
                STATIC_REQUIRE(
                    is_same_v<
                        std::invoke_result_t<
                            decltype(&operand::mult),
                            std::reference_wrapper<operand>,
                            int>,
                        int>);
#endif
            }
            SECTION("Pointer") {
                operand o(8);
                REQUIRE(::futures::detail::invoke(&operand::mult, &o, 1) == 8);
                STATIC_REQUIRE(
                    is_invocable_v<decltype(&operand::mult), operand*, int>);
                STATIC_REQUIRE(
                    is_same_v<
                        invoke_result_t<decltype(&operand::mult), operand*, int>,
                        int>);
#ifdef __cpp_lib_is_invocable
                STATIC_REQUIRE(
                    is_same_v<
                        std::invoke_result_t<
                            decltype(&operand::mult),
                            operand*,
                            int>,
                        int>);
#endif
            }
        }

        SECTION("Member variable") {
            STATIC_REQUIRE(std::is_member_pointer<
                           std::decay_t<decltype(&operand::mult)>>::value);
            SECTION("Self") {
                REQUIRE(invoke(&operand::num_, operand(9)) == 9);
                STATIC_REQUIRE(
                    is_invocable_v<decltype(&operand::num_), operand>);
                STATIC_REQUIRE(
                    is_same_v<
                        invoke_result_t<decltype(&operand::num_), operand>,
                        int&&>);
#ifdef __cpp_lib_is_invocable
                STATIC_REQUIRE(
                    is_same_v<
                        std::invoke_result_t<decltype(&operand::num_), operand>,
                        int&&>);
#endif
            }
            SECTION("Ref wrapper") {
                operand o(10);
                std::reference_wrapper<operand> r = std::ref(o);
                REQUIRE(::futures::detail::invoke(&operand::num_, r) == 10);
                STATIC_REQUIRE(
                    is_invocable_v<
                        decltype(&operand::num_),
                        std::reference_wrapper<operand>>);
                STATIC_REQUIRE(
                    is_same_v<
                        invoke_result_t<
                            decltype(&operand::num_),
                            std::reference_wrapper<operand>>,
                        int&>);
#ifdef __cpp_lib_is_invocable
                STATIC_REQUIRE(
                    is_same_v<
                        std::invoke_result_t<
                            decltype(&operand::num_),
                            std::reference_wrapper<operand>>,
                        int&>);
#endif
            }
            SECTION("Pointer") {
                operand o(11);
                REQUIRE(::futures::detail::invoke(&operand::num_, &o) == 11);
                STATIC_REQUIRE(
                    is_invocable_v<decltype(&operand::num_), operand*>);
                STATIC_REQUIRE(
                    is_same_v<
                        invoke_result_t<decltype(&operand::num_), operand*>,
                        int&>);
#ifdef __cpp_lib_is_invocable
                STATIC_REQUIRE(
                    is_same_v<
                        std::invoke_result_t<decltype(&operand::num_), operand*>,
                        int&>);
#endif
            }
        }
    }
}
