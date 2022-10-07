//
// Copyright (c) 2022 alandefreitas (alandefreitas@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//

#include "stress.hpp"
#include <futures/futures.hpp>
#include <iostream>

int
main(int argc, char** argv) {
    std::cout << TEST_CASE_PREFIX << "wait\n";
    return STRESS(argc, argv, [] {
        using namespace futures;
        constexpr auto enough_time_for_deadlock = std::chrono::milliseconds(20);
        auto f1 = async([t = enough_time_for_deadlock]() {
            std::this_thread::sleep_for(t);
            return 2;
        });
        auto f2 = async([t = enough_time_for_deadlock]() {
            std::this_thread::sleep_for(t);
            return 3.3;
        });
        auto f3 = async([t = enough_time_for_deadlock]() {
            std::this_thread::sleep_for(t);
            return;
        });
        std::size_t index = wait_for_any(f1, f2, f3);
        switch (index) {
        case 0:
        {
            int n2 = f1.get();
            (void) n2;
            break;
        }
        case 1:
        {
            double n2 = f2.get();
            (void) n2;
            break;
        }
        case 2:
        {
            f3.get();
            break;
        }
        }
    });
}