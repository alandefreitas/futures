# Launching tasks

Like [std::async], [futures::async] is used to launch new tasks. However, [futures::async] implements a few improvements and extensions to [std::async]:

- Its [launch] policy might be replaced with a concrete [executor]  
- If the [launch] policy or [executor] is defined, a default executor (a thread pool) is used for the tasks, instead of always launching a new thread
- It returns a continuable [cfuture] instead of a [std::future]
- If the first task parameter is a [stop_source], it returns a [jcfuture] 

```cpp
--8<-- "examples/launching.cpp"
```
 
## Executors

The concept defined for an [executor] uses Asio executors. This model for executors has been evolving for over a decade and is widely adopted in C++. If the C++ standard eventually adopts a common vocabulary for executors, the [executor] concept can be easily adjusted to handle these new executors.

## Exceptions

Future objects defined in this library handle exceptions the same way [std::future] does. If the task throws an exception internally, the exception is rethrown when we attempt to retrieve the value from the future. 

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

[is_future]: /futures/reference/Classes/structfutures_1_1is__future/
[jfuture]: /futures/reference/Namespaces/namespacefutures/#using-jfuture
[shared_jfuture]: /futures/reference/Namespaces/namespacefutures/#using-shared_jfuture
[cfuture]: /futures/reference/Namespaces/namespacefutures/#using-cfuture
[shared_cfuture]: /futures/reference/Namespaces/namespacefutures/#using-shared_cfuture
[jcfuture]: /futures/reference/Namespaces/namespacefutures/#using-jcfuture
[shared_jcfuture]: /futures/reference/Namespaces/namespacefutures/#using-shared_jcfuture
[stop_source]: /futures/reference/Classes/classfutures_1_1stop__source/
[stop_token]: /futures/reference/Classes/classfutures_1_1stop__token/