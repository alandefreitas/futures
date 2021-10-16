#include <futures/algorithm.h>
#include <futures/futures.h>
#include <iostream>

int main() {
    using namespace futures;

    // Direct
    auto f1 = async([]() { return 2; }) || async([]() { return 3.5; });
    auto f1c = then(f1, [](when_any_result<std::tuple<cfuture<int>, cfuture<double>>> r) {
        std::cout << r.index << " ready" << std::endl;
    });
    f1c.wait();

    // To tuple
    auto f2 = async([]() { return 2; }) || async([]() { return 3.5; });
    auto f2c = then(f2, [](size_t index, std::tuple<cfuture<int>, cfuture<double>>) {
        std::cout << index << " ready" << std::endl;
    });
    f2c.wait();

    // To futures
    auto f3 = async([]() { return 2; }) || async([]() { return 3.5; });
    auto f3c = then(f3, [](size_t index, cfuture<int>, cfuture<double>) {
        std::cout << index << " ready" << std::endl;
    });
    f3c.wait();

    // To ready future (when types are the same)
    auto f4 = async([]() { return 2; }) || async([]() { return 3; });
    auto f4c = then(f4, [](cfuture<int> v) {
        std::cout << v.get() << " returned" << std::endl;
    });
    f4c.wait();

    // To ready value (when types are the same)
    auto f5 = async([]() { return 2; }) || async([]() { return 3; });
    auto f5c = then(f5, [](int v) {
        std::cout << v << " returned" << std::endl;
    });
    f5c.wait();

    return 0;
}