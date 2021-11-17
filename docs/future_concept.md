# Futures

## Motivation 

A number of proposals have been presented to extend the model defined by the standard [std::future]: future continuations, cancellation tokens, association with executors, and algorithms. However, because a "future" object is any object representing a handle to a future value, it is highly unlikely that a single concrete future definition will be appropriate for most applications. 

In this context, this library implements future types as a *concept* rather than a concrete object. Algorithms work with any given class that has the requirements of the future concept, as defined by the trait [is_future]. The [std::future] class also has the requirements of this concept, so it can interoperate with any of the new algorithms and future types.

```cpp
--8<-- "examples/interoperability.cpp"
```
 

## Some future types

For instance, these are some concrete future classes defined in this library and their comparison with [std::future]:

| Class                | Shared | Lazy Continuations | Stoppable |
|----------------------|--------|--------------------|-----------|
| [std::future]        | No     | No                 | No        |
| [std::shared_future] | Yes    | No                 | No        |
| [jfuture]            | No     | No                 | Yes       |
| [shared_jfuture]     | Yes    | No                 | Yes       |
| [cfuture]            | No     | Yes                | No        |
| [shared_cfuture]     | Yes    | Yes                | No        |
| [jcfuture]           | No     | Yes                | Yes       |
| [shared_jcfuture]    | Yes    | Yes                | Yes       |
| [when_all_future]    | No     | No                 | No        |
| [when_any_future]    | No     | No                 | No        |

- Like [std::shared_future], multiple threads are allowed to wait for the same shared state of all shared future types. 
- Future types with lazy continuations allow new functions to be appended to the end of the current function being executed. This allows these continuations to run without launching a new task to the executor. If the continuation should run in a new executor, it allows the continuation to be scheduled only once the current task is completed.
- Stoppable future types contain an internal [stop_source], similar to [std::stop_source]. This allows the future to directly receive stop requests, while the internal future task can identify a stop request has been made through a [stop_token], similar to [std::stop_token]. This replicates the model of [std::jthread] for futures.    
- [when_all_future] and [when_any_future] are proxy future classes to hold the results of the [when_all] and [when_all] functions. This proxy allows different future types to interoperate and save resources on new tasks.

## Custom future types

Any custom type with the requirements of [is_future] can interoperate with other future type. These classes might represent any process for which a result will only be available in the future, such as child processes and network requests with third-party libraries (such as CURL).


[is_future]: /futures/reference/Classes/structfutures_1_1is__future/
[std::future]: https://en.cppreference.com/w/cpp/thread/future
[std::shared_future]: https://en.cppreference.com/w/cpp/thread/shared_future
[std::stop_source]: https://en.cppreference.com/w/cpp/thread/stop_source
[std::stop_token]: https://en.cppreference.com/w/cpp/thread/stop_token
[std::jthread]: https://en.cppreference.com/w/cpp/thread/jthread


--8<-- "docs/references.md"
