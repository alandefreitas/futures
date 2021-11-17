# Disjunctions

The function [when_any] (or [operator||]) is defined for task disjunctions. [when_any] returns a [when_any_future] that is able to aggregate different futures types and become ready when any of the internal futures is ready. 

```cpp
--8<-- "examples/disjunctions.cpp"
```

The [when_any_future] object acts as a proxy object that checks the state of each internal future. If none of the internal futures is ready yet, [is_ready] returns `false`.

## Lazy continuations

Unlike [when_all_future], the behavior of [when_any_future] is strongly affected by the [is_lazy_continuable] trait of its internal futures. Internal futures that support lazy continuations set a flag indicating to [when_any_future] that one of its futures is ready. 

For internal futures that do not support lazy continuations, [when_any_future] needs to pool its internal future to check if any is ready. A number of heuristics, such as exponential backoffs, are implemented to reduce the cost of polling.

## Disjunction unwrapping

Special unwrapping functions are defined for [when_any_future] continuations.

```cpp
--8<-- "examples/disjunctions_unwrap.cpp"
```

--8<-- "docs/references.md"