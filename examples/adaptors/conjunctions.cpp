#include <futures/futures.hpp>
#include <iostream>

int
main() {
    using namespace futures;

    {
        //[conjunction Future conjunction
        auto f1 = async([]() { return 2; });
        auto f2 = async([]() { return 3.5; });
        auto f3 = async([]() -> std::string { return "name"; });
        auto all = when_all(f1, f2, f3);
        //]

        //[conjunction_return Conjunction as tuple
        auto [r1, r2, r3] = all.get(); // get ready futures
        std::cout << r1.get() << '\n';
        std::cout << r2.get() << '\n';
        std::cout << r3.get() << '\n';
        //]
    }

    {
        //[conjunction_range Future conjunction as range
        std::vector<cfuture<int>> fs;
        fs.emplace_back(async([]() { return 2; }));
        fs.emplace_back(async([]() { return 3; }));
        fs.emplace_back(async([]() { return 4; }));
        auto all = when_all(fs);

        auto rs = all.get();
        std::cout << rs[0].get() << '\n';
        std::cout << rs[1].get() << '\n';
        std::cout << rs[2].get() << '\n';
        //]
    }

    {
        //[operator operator&&
        auto f1 = async([]() { return 2; });
        auto f2 = async([]() { return 3.5; });
        auto f3 = async([]() -> std::string { return "name"; });
        auto all = f1 && f2 && f3;
        //]

        //[continuation_unwrap Future conjunction as range
        auto f4 = then(all, [](int a, double b, std::string c) {
            std::cout << a << '\n';
            std::cout << b << '\n';
            std::cout << c << '\n';
        });
        //]
    }

    {
        // clang-format off
        //[operator_lambda Lambda conjunction
        auto f1 = []() { return 2; } &&
                  []() { return 3.5; } &&
                  []() -> std::string { return "name"; };

        auto f2 = then(f1, [](int a, double b, std::string c) {
            std::cout << a << '\n';
            std::cout << b << '\n';
            std::cout << c << '\n';
        });
        //]
        // clang-format on
    }

    return 0;
}