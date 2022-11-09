//
// Copyright (c) 2022 alandefreitas (alandefreitas@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//

//[include
#include <futures/futures.hpp>
//]

//[include_ind
#include <futures/launch.hpp>
//]

int
main() {
    return futures::async([] { return 0; }).get();
}