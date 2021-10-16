# Disjunctions

The function [when_any] (or [operator||]) is defined for task disjunctions. [when_any] returns a [when_any_future] that is able to aggregate different futures types and become ready when any of the internal futures is ready. 

```cpp
--8<-- "examples/disjunctions.cpp"
```

The [when_any_future] object acts as a proxy object that checks the state of each internal future. If none of the internal futures is ready yet, [is_ready] returns `false`.

## Lazy continuations

Unlike [when_all_future], the behavior of [when_any_future] is strongly affected by the [is_lazy_continuable] trait of its internal futures. Internal futures that support lazy continuations set a flag indicating to [when_any_future] that one of its futures is ready. 

For internal futures that do not support lazy continuations, [when_any_future] needs to pool its internal future to check if any is ready. A number of heuristics, such as exponential backoffs, are implemented to reduce the cost of polling.

## Disjunction unwrapping

Special unwrapping functions are defined for [when_any_future] continuations.

```cpp
--8<-- "examples/disjunctions_unwrap.cpp"
```

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

[C++ Extensions for Concurrency]: https://en.cppreference.com/w/cpp/experimental/concurrency
[std::experimental::when_all]: https://en.cppreference.com/w/cpp/experimental/when_all
[std::experimental::when_any]: https://en.cppreference.com/w/cpp/experimental/when_any