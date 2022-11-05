# Algorithms

The header [`algorithm.h`](/futures/reference/Files/algorithm_8h.md) and the corresponding
[Algorithms Module](reference/Modules/group__algorithms/) includes parallel implementations of common STL algorithms
using the library primitives.

{{ code_snippet("algorithm/algorithms.cpp", "algorithm") }}

Like the C++20 [ranges library](https://en.cppreference.com/w/cpp/ranges), these algorithms accept both iterators or
ranges as parameters.

{{ code_snippet("algorithm/algorithms.cpp", "algorithm_range") }}

!!! info "Better future algorithms"

    This library includes:

    - Large set of the STL-like algorithms in terms of futures and executors
    - Easy access to async algorithms based on executors without requiring other external libraries
        - This is [common](https://link.springer.com/chapter/10.1007/978-1-4842-4398-5_4) with C++ execution policies, such
          as [TTB].


## Executors

Like other parallel functions defined in this library, these algorithms allow simple execution policies to be replaced
by concrete executors.

{{ code_snippet("algorithm/algorithms.cpp", "executor") }}

## Parallel by default

Unless an alternative policy or [executor] is provided, all algorithms are executed in parallel by default whenever it
is "reasonably safe" to do so. A parallel algorithm is considered "reasonably safe" if there are no implicit data races
and deadlocks in its provided functors.

To execute algorithms sequentially, an appropriate executor or policy should be provided:

{{ code_snippet("algorithm/algorithms.cpp", "inline_executor") }}

{{ code_snippet("algorithm/algorithms.cpp", "seq_policy") }}

Unless a policy is explicitly stated, all algorithms are parallel by default. These algorithms give us access to
parallel algorithms that rely only on executors. This allows us to avoid of more complex libraries, such
as [TBB](https://github.com/oneapi-src/oneTBB), to execute efficient parallel algorithms.

## Compile time algorithms

Like in [C++20](https://en.cppreference.com/w/cpp/algorithm), these algorithms can also be used in
`constexpr` contexts with the default inline executor for these contexts.

{{ code_snippet("algorithm/algorithms.cpp", "constexpr") }}

This feature depends on an internal library implementation equivalent to [std::is_constant_evaluated]. This
implementation is available in most compilers (MSVC 1925, GCC 6, Clang 9), even when C++20 is not available. The macro
`FUTURES_HAS_CONSTANT_EVALUATED` can be used to identify if the feature is available.

--8<-- "docs/references.md"