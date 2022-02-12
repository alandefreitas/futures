#include <futures/algorithm.hpp>
#include <futures/futures.hpp>
#include <iostream>

void
task_that_might_fail() {}

void
some_task() {
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
}

int
main() {
    using namespace futures;

    {
        //[error Continuations with no unwrapping
        cfuture<void> f1 = async([]() { task_that_might_fail(); });
        cfuture<void> f2 = then(f1, [](cfuture<void> f) {
            if (!f.get_exception_ptr()) {
                std::cout << "Success\n";
            } else {
                // handle error
            }
        });
        //]
    }

    {
        //[verbose_create Task returning 3 values
        auto f1 = async([]() {
            return std::make_tuple(
                make_ready_future(1),
                make_ready_future(2.0),
                make_ready_future<std::string>("3"));
        });
        //]
        //[verbose_continue Verbose continuations
        cfuture<void> f2 = then(
            f1,
            [](cfuture<std::tuple<
                   vfuture<int>,
                   vfuture<double>,
                   vfuture<std::string>>> f) {
            // retrieve futures
            auto t = f.get();
            vfuture<int> fa = std::move(std::get<0>(t));
            vfuture<double> fb = std::move(std::get<1>(t));
            vfuture<std::string> fc = std::move(std::get<2>(t));
            // get their values
            int a = fa.get();
            double b = fb.get();
            std::string c = fc.get();
            // use values
            std::cout << a << ' ' << b << ' ' << c << '\n';
            });
        //]
    }

    {
        // clang-format off
        //[unwrap_void Unwrap void
        cfuture<void> f1 = async([]() { std::cout << "Task\n"; });
        cfuture<int> f2 = f1 >> []() { return 6; };
        //]
        // clang-format on
    }


    {
        //[unwrap_fail_void Unwrap and query
        cfuture<void> f1 = async([]() { task_that_might_fail(); });

        cfuture<int> f2 = f1 >> []() {
            return 6;
        };

        if (!f2.get_exception_ptr()) {
            std::cout << f2.get() << '\n';
        } else {
            // handle error
        }
        //]
    }

    {
        // clang-format off
        //[value_unwrap Forward value directly
        auto f1 = async([]() { return 6; });
        auto f2 = f1 >> [](int x) { return x * 2; };
        //]
        // clang-format on
        std::cout << f2.get() << '\n';
    }

    {
        // clang-format off
        //[double_unwrap Double value unwrap
        auto f1 = async([]() { return make_ready_future(6); });
        auto f2 = f1 >> [](int x) { return x * 2; };
        //]
        // clang-format on
        std::cout << f2.get() << '\n';
    }

    {
        //[tuple_unwrap Unwrap tuple
        auto f1 = async([]() { return make_ready_future(6); });
        auto f2 = f1 >> [](int x) {
            return std::make_tuple(x * 1, x * 2, x * 3);
        };
        cfuture<int> f3 = f2 >> [](int a, int b, int c) {
            return a * b * c;
        };
        std::cout << f3.get() << '\n'; // 6*1*6*2*6*3
        //]
    }

    {
        //[double_tuple_unwrap Unwrap tuple
        auto f1 = async([]() { return make_ready_future(6); });
        auto f2 = f1 >> [](int x) {
            return std::make_tuple(
                make_ready_future(1 * x),
                make_ready_future(2 * x),
                make_ready_future(3 * x));
        };
        auto f3 = f2 >> [](int a, int b, int c) {
            return a + b + c;
        };
        std::cout << f3.get() << '\n'; // 1*6 + 2*6 + 3*6
        //]
    }

    {
        //[when_all_unwrap Unwrap future conjunction
        auto f1 = async([]() { return 1; });
        auto f2 = async([]() { return 2; });
        auto f3 = async([]() { return 3; });
        auto f4 = async([]() { return 4; });
        auto f5 = when_all(f1, f2, f3, f4);
        auto f6 = f5 >> [](int a, int b, int c, int d) {
            return a + b + c + d;
        };
        std::cout << f6.get() << '\n';
        //]
    }

    {
        //[when_all_unwrap Unwrap future conjunction
        auto f1 = async([]() { return 1; });
        auto f2 = async([]() { return 2; });
        auto f3 = async([]() { return 3; });
        auto f4 = async([]() { return 4; });
        auto f5 = when_all(f1, f2, f3, f4);
        auto f6 = f5 >> [](int a, int b, int c, int d) {
            return a + b + c + d;
        };
        std::cout << f6.get() << '\n';
        //]
    }

    {
        //[when_any_unwrap Unwrap future disjunction
        cfuture<int> f1 = async([]() { return 1; });
        cfuture<int> f2 = async([]() { return 2; });
        when_any_future<std::tuple<cfuture<int>, cfuture<int>>>
            f3 = when_any(f1, f2);
        auto f4 = f3 >>
                  [](std::size_t idx,
                     std::tuple<cfuture<int>, cfuture<int>> prev) {
            if (idx == 0) {
                return std::get<0>(prev).get();
            } else {
                return std::get<1>(prev).get();
            }
        };
        std::cout << f4.get() << '\n';
        //]
    }

    {
        //[when_any_explode_unwrap Explode future conjunction
        auto f1 = async([]() { return 1; });
        auto f2 = async([]() { return 2; });
        auto f3 = when_any(f1, f2);
        auto f4 = f3 >> [](std::size_t idx,
                           cfuture<int> f1,
                           cfuture<int> f2) {
            if (idx == 0) {
                return f1.get();
            } else {
                return f2.get();
            }
        };
        std::cout << f4.get() << '\n';
        //]
    }

    {
        //[when_any_single_result Homogenous disjunction
        auto f1 = async([]() { return 1; });
        auto f2 = async([]() { return 2; });
        auto f3 = when_any(f1, f2);
        auto f4 = f3 >> [](cfuture<int> f) {
            return f.get();
        };
        std::cout << f4.get() << '\n'; // 1 or 2
        //]
    }

    {
        //[when_any_single_result_unwrap Homogenous unwrap
        auto f1 = async([]() { return 1; });
        auto f2 = async([]() { return 2; });
        auto f3 = when_any(f1, f2);
        auto f4 = f3 >> [](int v) {
            return v * 2;
        };
        std::cout << f4.get() << '\n'; // 2 or 4
        //]
    }

    {
        //[ambiguous Ambiguous unwrapping
        auto f1 = async([]() { return 1; });
        auto f2 = f1 >> [](auto f) {
             // Is `f` a `cfuture<int>` or `int`?
             // `cfuture<int>` has the highest priority
             return f.get();
        };
        std::cout << f2.get() << '\n';
        //]
    }

    {
        //[return_future Getting a future from a future
        cfuture<cfuture<int>> f = async([]() {
            return async([]() {
                return 1;
            });
        });
        std::cout << f.get().get() << '\n';
        //]
    }

    {
        //[stop_source Continuation stop source
        auto f1 = async([](stop_token st) {
            while (!st.stop_requested()) {
                some_task();
            }
        });
        auto ss = f1.get_stop_source();
        auto f2 = f1 >> []() {
            std::cout << "Done\n";
        };
        ss.request_stop(); // f1.request_stop() won't work anymore
        f2.get();
        //]
    }

    return 0;
}