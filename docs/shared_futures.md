# Shared Futures

Like [std::future] and [std::shared_future], the classes [jfuture], [cfuture], [jcfuture] also have their shared counterparts [shared_jfuture], [shared_cfuture], [shared_jcfuture].

In a shared future, multiple threads are allowed to wait for the same shared state of all shared future types. The value does not need to be moved and can be accessed from any future copy:

```cpp
--8<-- "examples/future_types/shared.cpp"
```

--8<-- "docs/references.md"