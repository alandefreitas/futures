#include <futures/algorithm.h>
#include <futures/futures.h>
#include <iostream>

int main() {
    using namespace futures;

    // Tuple disjunction
    cfuture<int> f1 = async([]() { return 2; });
    cfuture<double> f2 = async([]() { return 3.5; });
    cfuture<std::string> f3 =
        async([]() -> std::string { return "name"; });
    auto f = when_any(f1, f2, f3); // or f1 || f2 || f3

    // Get result
    when_any_result any_r = f.get();
    size_t i = any_r.index;
    auto [r1, r2, r3] = std::move(any_r.tasks);
    if (i == 0) {
        std::cout << "int is ready: " << r1.get() << std::endl;
    } else if (i == 1) {
        std::cout << "double is ready: " << r2.get() << std::endl;
    } else {
        std::cout << "string is ready: " << r3.get() << std::endl;
    }

    return 0;
}