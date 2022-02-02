#include <futures/algorithm.hpp>
#include <futures/futures.hpp>
#include <iostream>

int
main() {
    using namespace futures;

    // Tuple conjunction
    auto f1 = async([]() { return 2; });
    auto f2 = async([]() { return 3.5; });
    auto f3 = async([]() -> std::string { return "name"; });
    auto fc1 = when_all(f1, f2, f3);

    // Operator&&
    auto f4 = async([]() { return 2; });
    auto f5 = async([]() { return 3.5; });
    auto f6 = async([]() -> std::string { return "name"; });
    auto fc2 = f4 && f5 && f6;

    // r1, r2, r3 are also futures
    auto [r11, r21, r31] = fc1.get();
    std::cout << r11.get() << '\n';
    std::cout << r21.get() << '\n';
    std::cout << r31.get() << '\n';

    auto [r12, r22, r32] = fc2.get();
    std::cout << r12.get() << '\n';
    std::cout << r22.get() << '\n';
    std::cout << r32.get() << '\n';

    return 0;
}