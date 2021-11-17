# Continuations

Futures can have continuations, which allows us to create tasks chains. The function [then] is used to create a continuation to a future, which is itself another future.

```cpp
--8<-- "examples/adaptors/continuations.cpp"
```

!!! note "Continuation Function"

    Only futures with lazy continuable have a `then` member function. For the general case, we should use the free function [then].

## Lazy continuations

The function [then] changes its behavior according to the traits [is_lazy_continuable] defined for the previous future type. If a previous future type supports lazy continuations, the next task is attached to the previous task. If the previous type does not support lazy continuations, a new tasks that waits for the previous tasks is sent to the executor.   

## Continuation unwrapping

Because continuations involve accessing the future object for the previous tasks, creating continuation chains can become verbose. To simplify this process, the function [then] accepts continuations that expect the unwrapped result from the previous task.

This following example include the basic wrapping functions.

```cpp
--8<-- "examples/adaptors/continuations_unwrap.cpp"
```

## Continuation stop

Like the [async] function, the function [then] might return a future type with a [stop_token]. This happens whenever 1) the continuation function expects a stop token, or 2) the previous future has a stop token. In the second case, both futures with share the same [stop_source].    

--8<-- "docs/references.md"