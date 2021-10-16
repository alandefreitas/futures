# Shared Futures

Like [std::future] and [std::shared_future], the classes [jfuture], [cfuture], [jcfuture] also have their shared counterparts [shared_jfuture], [shared_cfuture], [shared_jcfuture].

In a shared future, multiple threads are allowed to wait for the same shared state of all shared future types. The value does not need to be moved and can be accessed from any future copy:

```cpp
--8<-- "examples/shared.cpp"
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