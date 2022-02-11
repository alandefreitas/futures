#include <futures/algorithm.hpp>
#include <futures/futures.hpp>
#include <iostream>

int
main() {
    using namespace futures;

    {
        //[small_graph Task graph with conjunction
        shared_cfuture<int> A = async([]() { return 2; }).share();

        cfuture<int> B = then(A, [](int a) { return a * 2; });
        cfuture<int> C = then(A, [](int a) { return a * 3; });

        cfuture<int> D = then(when_any(B, C), [](int b_or_c) {
            return b_or_c * 3;
        });

        std::cout << D.get() << '\n'; // 12 or 18
        //]
    }

    {
        //[disjunction Tuple disjunction
        cfuture<int> f1 = async([]() { return 2; });
        cfuture<double> f2 = async([]() { return 3.5; });
        cfuture<std::string> f3 = async([]() -> std::string {
            return "name";
        });
        auto f = when_any(f1, f2, f3); // or f1 || f2 || f3
        //]

        //[disjunction_result Get disjunction result
        when_any_result any_r = f.get();
        size_t i = any_r.index;
        auto [r1, r2, r3] = std::move(any_r.tasks);
        if (i == 0) {
            std::cout << r1.get() << '\n';
        } else if (i == 1) {
            std::cout << r2.get() << '\n';
        } else {
            std::cout << r3.get() << '\n';
        }
        //]
    }

    {
        //[operator operator||
        cfuture<int> f1 = async([]() { return 2; });
        cfuture<int> f2 = async([]() { return 3; });
        cfuture<int> f3 = async([]() { return 4; });
        auto any = f1 || f2 || f3;
        //]

        // clang-format off
        //[disjunction_result_unwrap Get disjunction result
        auto f4 = any >> [](int first) { return first; };
        std::cout << f4.get() << '\n';
        //]
        // clang-format on
    }

    {
        // clang-format off
        //[operator_lambda Lambda conjunction
        auto f1 = []() { return 2; } ||
                  []() { return 3; } ||
                  []() { return 4; };
        auto f2 = then(f1, [](int first) {
            std::cout << first << '\n';
        });
        //]
        // clang-format on
    }

    return 0;
}