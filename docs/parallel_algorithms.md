# Parallel Algorithms

The header [`algorithm.h`](reference/Files/algorithm_8h.md) includes implementations of common STL algorithms using these primitives. These algorithms also accept ranges as parameters and, unless a policy is explicitly stated, all algorithms are parallel by default.

```cpp
--8<-- "examples/algorithms.cpp"
```

These algorithms give us access to parallel algorithms that rely only on executors, instead of more complex libraries, such as TBB. Like other parallel algorithms defined in this library, these algorithms can also accept executors instead of policies as parameters.
  
## Partitioners

All algorithms also have an optional extra parameter for the algorithms can also accept custom partitioners instead of the [default_partitioner]. A [partitioner] is simply a callable object that receives two iterators representing a range and returns an iterator indicating where this range should be split for a parallel algorithm.

[std::future]: https://en.cppreference.com/w/cpp/thread/future
[std::shared_future]: https://en.cppreference.com/w/cpp/thread/shared_future
[std::stop_source]: https://en.cppreference.com/w/cpp/thread/stop_source
[std::stop_token]: https://en.cppreference.com/w/cpp/thread/stop_token
[std::jthread]: https://en.cppreference.com/w/cpp/thread/jthread
[std::async]: https://en.cppreference.com/w/cpp/thread/async

[async]: /futures/reference/Namespaces/namespacefutures/#function-async
[futures::async]: /futures/reference/Namespaces/namespacefutures/#function-async
[launch]: /futures/reference/Namespaces/namespacefutures/#enum-launch
[executor]: /futures/reference/Namespaces/namespacefutures/#using-is_executor
[then]: /futures/reference/Namespaces/namespacefutures/#function-then
[is_ready]: /futures/reference/Namespaces/namespacefutures/#function-is_ready
[is_lazy_continuable]: /futures/reference/Classes/structfutures_1_1is__lazy__continuable/

[is_future]: /futures/reference/Classes/structfutures_1_1is__future/
[jfuture]: /futures/reference/Namespaces/namespacefutures/#using-jfuture
[shared_jfuture]: /futures/reference/Namespaces/namespacefutures/#using-shared_jfuture
[cfuture]: /futures/reference/Namespaces/namespacefutures/#using-cfuture
[shared_cfuture]: /futures/reference/Namespaces/namespacefutures/#using-shared_cfuture
[jcfuture]: /futures/reference/Namespaces/namespacefutures/#using-jcfuture
[shared_jcfuture]: /futures/reference/Namespaces/namespacefutures/#using-shared_jcfuture
[stop_source]: /futures/reference/Classes/classfutures_1_1stop__source/
[stop_token]: /futures/reference/Classes/classfutures_1_1stop__token/
[when_all_future]: /futures/reference/Classes/classfutures_1_1when__all__future/
[when_any_future]: /futures/reference/Classes/classfutures_1_1when__any__future/
[when_all]: /futures/reference/Namespaces/namespacefutures/#function-when_all
[when_any]: /futures/reference/Namespaces/namespacefutures/#function-when_any
[operator&&]: /futures/reference/Namespaces/namespacefutures/#function-operator&&
[operator||]: /futures/reference/Namespaces/namespacefutures/#function-operator||
[is_partitioner]: /futures/reference/Namespaces/namespacefutures/#using-is_partitioner
[partitioner]: /futures/reference/Namespaces/namespacefutures/#using-is_partitioner
[default_partitioner]: /futures/reference/Namespaces/namespacefutures/#using-default_partitioner

[C++ Extensions for Concurrency]: https://en.cppreference.com/w/cpp/experimental/concurrency
[std::experimental::when_all]: https://en.cppreference.com/w/cpp/experimental/when_all
[std::experimental::when_any]: https://en.cppreference.com/w/cpp/experimental/when_any