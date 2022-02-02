#include <futures/algorithm.hpp>
#include <futures/futures.hpp>
#include <iostream>

int
main() {
    using namespace futures;

    shared_cfuture<int> f1 = async([] { return 1; }).share();

    // OK to copy
    shared_cfuture<int> f2 = f1;

    // OK to get
    std::cout << f1.get() << '\n';

    // OK to call get on the copy
    std::cout << f2.get() << '\n';

    // OK to call get twice
    std::cout << f1.get() << '\n';
    std::cout << f2.get() << '\n';

    return 0;
}