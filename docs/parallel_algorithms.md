# Parallel Algorithms

The header [`algorithm.h`](reference/Files/algorithm_8h.md) includes implementations of common STL algorithms using these primitives. These algorithms also accept ranges as parameters and, unless a policy is explicitly stated, all algorithms are parallel by default.

```cpp
--8<-- "examples/algorithm/algorithms.cpp"
```

These algorithms give us access to parallel algorithms that rely only on executors, instead of more complex libraries, such as TBB. Like other parallel algorithms defined in this library, these algorithms can also accept executors instead of policies as parameters.
  
## Partitioners

All algorithms also have an optional extra parameter for the algorithms can also accept custom partitioners instead of the [default_partitioner]. A [partitioner] is simply a callable object that receives two iterators representing a range and returns an iterator indicating where this range should be split for a parallel algorithm.




--8<-- "docs/references.md"