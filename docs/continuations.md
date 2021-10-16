# Continuations

Futures can have continuations, which allows us to create tasks chains. The function [then] is used to create a continuation to a future, which is itself another future.

```cpp
--8<-- "examples/continuations.cpp"
```

!!! note "Continuation Function"

    Only futures with lazy continuable have a `then` member function. For the general case, we should use the free function [then].

## Lazy continuations

The function [then] changes its behavior according to the traits [is_lazy_continuable] defined for the previous future type. If a previous future type supports lazy continuations, the next task is attached to the previous task. If the previous type does not support lazy continuations, a new tasks that waits for the previous tasks is sent to the executor.   

## Continuation unwrapping

Because continuations involve accessing the future object for the previous tasks, creating continuation chains can become verbose. To simplify this process, the function [then] accepts continuations that expect the unwrapped result from the previous task.

This following example include the basic wrapping functions.

```cpp
--8<-- "examples/continuations.cpp"
```

## Continuation stop

Like the [async] function, the function [then] might return a future type with a [stop_token]. This happens whenever 1) the continuation function expects a stop token, or 2) the previous future has a stop token. In the second case, both futures with share the same [stop_source].    

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