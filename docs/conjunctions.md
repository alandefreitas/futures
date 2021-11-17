# Conjunctions

Like in [C++ Extensions for Concurrency], the [when_all] function (or [operator&&]) is defined for task conjunctions. The function [when_all] return a [when_all_future] that is able to aggregate different futures types and become ready when all internal futures are ready. 

```cpp
--8<-- "examples/adaptors/conjunctions.cpp"
```

The [when_all_future] object acts as a proxy object that checks the state of each internal future. If any of the internal futures isn't ready yet, [is_ready] returns `false`.

## Conjunction unwrapping

Special unwrapping functions are defined for [when_all_future] continuations.

```cpp
--8<-- "examples/adaptors/conjunctions_unwrap.cpp"
```

--8<-- "docs/references.md"