//
// Copyright (c) 2022 alandefreitas (alandefreitas@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//

#include <futures/launch.hpp>
#include <iostream>
#include "stress.hpp"

int
main(int argc, char** argv) {
    std::cout << TEST_CASE_PREFIX << "launch\n";
    return STRESS(argc, argv, []{
        using namespace futures;
        auto f = schedule([]() { return 2; });
        assert(f.get() == 2);
    });
}