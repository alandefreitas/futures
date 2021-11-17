#include <futures/algorithm.h>
#include <futures/futures.h>
#include <iostream>

int main() {
    using namespace futures;

    shared_cfuture<int> f1 = async([] { return 1; }).share();

    // OK to copy
    shared_cfuture<int> f2 = f1;

    // OK to get
    std::cout << f1.get() << std::endl;

    // OK to call get on the copy
    std::cout << f2.get() << std::endl;

    // OK to call get twice
    std::cout << f1.get() << std::endl;
    std::cout << f2.get() << std::endl;

    return 0;
}