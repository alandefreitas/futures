# Stoppable Futures

When calling [async] with a callable that can be called with a [stop_token] as its first argument, it returns [jcfuture], which contains a [stop_source]:  
 
```cpp
--8<-- "examples/future_types/stoppable.cpp"
```

--8<-- "docs/references.md"