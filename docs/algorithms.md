# Algorithms

The header [`algorithm.h`](reference/Files/algorithm_8h.md) includes implementations of common STL algorithms using the
library primitives.

{{ code_snippet("algorithm/algorithms.cpp", "algorithm") }}

Like the C++20 [ranges library](https://en.cppreference.com/w/cpp/ranges), these algorithms accept both iterators or
ranges as parameters.

Unless a policy is explicitly stated, all algorithms are parallel by default. These algorithms give us access to
parallel algorithms that rely only on executors. This allows us to avoid of more complex libraries, such
as [TBB](https://github.com/oneapi-src/oneTBB), to execute efficient parallel algorithms.

Like other parallel functions defined in this library, these algorithms allow simple execution policies to be replaced
by concrete executors.

{{ code_snippet("algorithm/algorithms.cpp", "executor") }}

!!! warning "Parallel by default"

    Unless an alternative policy or [executor] is provided, all algorithms are executed in parallel by default whenever
    it is "reasonably safe" to do so. A parallel algorithms is considered "reasonably safe" if there are no implicit 
    data races and deadlocks in its provided functors. 

!!! hint "[constexpr]"

    Unlike [C++ algorithms](https://en.cppreference.com/w/cpp/algorithm), async algorithms using runtime executors and
    schedulers cannot be executed as [constexpr]. However, in C++20, algorithms might still be declared [constexpr] and 
    make use of [std::is_constant_evaluated].

--8<-- "docs/references.md"