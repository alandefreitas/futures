# Futures types

## Concrete types

This library implements a number of future types as a *concept* rather than a single concrete object.
For instance, consider a task we are launching with C++11 [std::async]:

{{ code_snippet("future_types/interoperability.cpp", "std_async") }}

We could do something similar with the library function [futures::async]:

{{ code_snippet("future_types/interoperability.cpp", "cfuture") }}

Notice how [async] returns a [cfuture] by default, which is a future to which we can attach continuations.
The function [async] does not always return [cfuture]. For instance, suppose we call [async] with a
function that accepts a [stop_token]:

{{ code_snippet("future_types/interoperability.cpp", "jcfuture") }}

It now returns [jcfuture] by default, which is a future type that supports stop tokens.
We can ask the task to stop through the future at any time with:

{{ code_snippet("future_types/interoperability.cpp", "jcfuture_stop") }}

We now have three types of future objects. Because all of these types has the requirements of the [is_future] concept,
the library functions allow them to interoperate:

{{ code_snippet("future_types/interoperability.cpp", "wait_for_all") }}

## Summary

For instance, these are some concrete future classes defined in this library and their comparison with [std::future]:

| Class             | Lazy Continuations | Stoppable | Shared |
|-------------------|--------------------|-----------|--------|
| [future]          | ❌                  | ❌         | ❌     |
| [shared_future]   | ❌                  | ❌         | ✅     |
| [jfuture]         | ❌                  | ✅         | ❌     |
| [shared_jfuture]  | ❌                  | ✅         | ✅     |
| [cfuture]         | ✅                  | ❌         | ❌     |
| [shared_cfuture]  | ✅                  | ❌         | ✅     |
| [jcfuture]        | ✅                  | ✅         | ❌     |
| [shared_jcfuture] | ✅                  | ✅         | ✅     |

- Like [std::shared_future], multiple threads are allowed to wait for the same shared state of all shared future types.
- Future types with lazy continuations allow new functions to be appended to the end of the current function being
  executed. This allows these continuations to run without launching a new task to the executor. If the continuation
  should run in a new executor, it allows the continuation to be scheduled only once the current task is completed.
- Stoppable future types contain an internal [stop_source], similar to [std::stop_source]. This allows the future to
  directly receive stop requests, while the internal future task can identify a stop request has been made through
  a [stop_token], similar to [std::stop_token]. This replicates the model of [std::jthread] for futures.

## Adaptor types

Some other future types are:

| Class             | Description                                   |
| ----------------- |-----------------------------------------------|
| **Value future**  |                                               | 
| [vfuture]         | Hold a single value as a future               |
| **Adaptors**      |                                               | 
| [when_all_future] | Represent the conjunction of other futures    | 
| [when_any_future] | Represent the disjunction of other futures    | 

Unlike other future types, [when_all_future] and [when_any_future] are proxy future classes to hold the results of
the [when_all] and [when_all] functions. This proxy allows different future types to interoperate and save resources on
new tasks.

## Custom future types

Any custom type with the requirements of [is_future] can interoperate with other future type. These classes might
represent any process for which a result will only be available in the future, such as child processes and network
requests with third-party libraries.

A simpler alternative for custom future types is through specific template instantiations of [basic_future], which can
be configured at compile-time with [future_options]. Many future types provided by the library as aliases [basic_future]
with specific [future_options].

--8<-- "docs/references.md"
