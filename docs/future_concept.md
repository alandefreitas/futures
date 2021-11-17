
# Futures Concepts

This library implements a number of future types as a *concept* rather than a single concrete object. Algorithms work with any given class that has the requirements of the future concept, as defined by the trait [is_future]. The [std::future] class also has the requirements of this concept, so it can interoperate with any of the new algorithms and future types.

```cpp
--8<-- "examples/future_types/interoperability.cpp"
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

--8<-- "docs/references.md"
