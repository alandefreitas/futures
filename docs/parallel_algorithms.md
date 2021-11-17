# Parallel Algorithms

The header [`algorithm.h`](reference/Files/algorithm_8h.md) includes implementations of common STL algorithms using these primitives. These algorithms also accept ranges as parameters and, unless a policy is explicitly stated, all algorithms are parallel by default.

```cpp
--8<-- "examples/algorithm/algorithms.cpp"
```

These algorithms give us access to parallel algorithms that rely only on executors, instead of more complex libraries, such as TBB. Like other parallel algorithms defined in this library, these algorithms can also accept executors instead of policies as parameters.

!!! warning "Parallel by default"

    Unless an alternative policy or [executor] is provided, all algorithms are executed in parallel by default whenever it is "reasonably safe" to do so. A parallel algorithms is considered "reasonably safe" if there are no implicit data races and deadlocks in its provided functors. 


!!! hint "[constexpr]"

    Unlike [C++ algorithms](https://en.cppreference.com/w/cpp/algorithm), async algorithms using runtime executors and schedulers cannot be executed as [constexpr]. However, the functions might still be declared [constexpr] and make use of [std::is_constant_evaluated].


## Partitioners

All algorithms also have an optional extra parameter for the algorithms can also accept custom partitioners instead of the [default_partitioner]. A [partitioner] is simply a callable object that receives two iterators representing a range and returns an iterator indicating where this range should be split for a parallel algorithm.




--8<-- "docs/references.md"