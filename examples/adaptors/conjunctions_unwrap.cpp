#include <futures/algorithm.hpp>
#include <futures/futures.hpp>
#include <iostream>

int
main() {
    using namespace futures;

    // Direct
    auto f1 = async([]() { return 2; })
              && async([]() { return 3.5; });
    auto f1c
        = then(f1, [](std::tuple<cfuture<int>, cfuture<double>> r) {
              return std::get<0>(r).get()
                     + static_cast<int>(std::get<1>(r).get());
          });
    std::cout << f1c.get() << '\n';

    // Unwrap tuple
    auto f2 = async([]() { return 2; })
              && async([]() { return 3.5; });
    auto f2c = then(f2, [](cfuture<int> r1, cfuture<double> r2) {
        return r1.get() + static_cast<int>(r2.get());
    });
    std::cout << f2c.get() << '\n';

    // Unwrap values
    auto f3 = async([]() { return 2; })
              && async([]() { return 3.5; });
    auto f3c = then(f3, [](int r1, double r2) {
        return r1 + static_cast<int>(r2);
    });
    std::cout << f3c.get() << '\n';

    return 0;
}