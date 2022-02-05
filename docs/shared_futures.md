# Shared Futures

Like [std::future] and [std::shared_future], the classes [jfuture], [cfuture], [jcfuture] also have their shared counterparts [shared_jfuture], [shared_cfuture], [shared_jcfuture].

In a shared future, multiple tasks are allowed to wait for and depend on the shared state with another promise. 

<div class="mermaid">
graph TB
F1[Shared Future] --> |read|S[(Shared State)]
F2[Shared Future] --> |read|S[(Shared State)]
F3[...] --> |read|S[(Shared State)]
F4[Shared Future] --> |read|S[(Shared State)]
P[Promise] --> |write|S
</div>

```cpp
--8<-- "examples/future_types/shared.cpp"
```

While a regular future object is invalidated when its value is read, a value shared by
many futures can be read multiple times. The main consideration when using a shared
future is that the value returned by the
[`get`](/futures/reference/Classes/classfutures_1_1basic__future/#function-get) function
is not moved outside the future.

This means this reference needs to be handled carefully, which often involves copying the value.
Having say that, there are applications where sharing the results from the future are simply
necessary.

--8<-- "docs/references.md"