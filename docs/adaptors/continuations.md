# Continuations

Continuations are the main primitive to assemble asynchronous task graphs. An instance of [basic_future] can have
continuations, which allows us to create tasks chains.

Say we want to execute the following sequence of asynchronous tasks:

<div class="mermaid">
graph LR
subgraph Async
A --> B --> C
end
Main --> A
C --> End
Main --> End
</div>

Achieving that with the function [then] is as simple as:

{{ code_snippet("future_types/continuable.cpp", "continuables") }}

All instances of [basic_future] that support lazy continuations also have the member function [basic_future::then].
However, the free function [then] is used to create a continuation to any future, which is itself another future value.

{{ code_snippet("adaptors/continuations.cpp", "continuable") }}

{{ code_snippet("adaptors/continuations.cpp", "std_future") }}

!!! hint "Continuation Function"

    Only futures with lazy continuations have a [basic_future::then] member function. For the general case, we should 
    prefer the free function [then].

The free function [then] allows us to create task graphs with future continuations regardless of underlying support for
lazy continuations.

## Lazy continuations

The function [then] changes its behavior according to the traits [is_continuable] defined for the previous future type.
If a previous future type supports lazy continuations, the next task is attached to the previous task with
[basic_future::then].

If the next future type [is_deferred], then no lazy continuations need to be involved. A new deferred task waits for the
previous task to be ready inline. Only when the previous task is ready the continuation task will be launched to the
executor.

{{ code_snippet("adaptors/continuations.cpp", "deferred") }}

In both cases, there's no polling involved. Polling is only necessary for (i) eager futures, (ii) that don't support
continuations, and (iii) are potentially not ready. The last criteria eliminates future types such as [vfuture]
generated by [make_ready_future].

In general, such futures types should not be used when we require continuations. However, to enable generic algorithms,
the function [then] also works for these future types and will automatically launch polling tasks to wait for their
results.

{{ code_snippet("adaptors/continuations.cpp", "std_future") }}

## Executors

It's important to note continuations are never executed inline. Although common patterns used in javascript for callback
functions are still possible, future continuations are always posted to the executor again. The default future objects
returned by [futures::async] carry light handles to their execution contexts, through which continuation tasks can be
launched by default. If a future object carries no executor, the default executor is used.

However, if the continuation should be launched with another executor, both the member function [basic_future::then] and
the free function [then] support custom executors for the continuation task.

{{ code_snippet("adaptors/continuations.cpp", "executor") }}

When this parameter is provided, the task will continue in another executor.

## Operators

The operator `>>` is defined as a convenience to assemble large task graphs including continuations.

{{ code_snippet("adaptors/continuations.cpp", "operator") }}

{{ code_snippet("adaptors/continuations.cpp", "operator_ex") }}

The types accepted by these operators are limited to those matching the future concept and callables that are valid as
continuations to the future instance.

## Continuation unwrapping

By default, continuations attempt to receive the previous future object as their input. This allows the continuation to
examine the state of the previous future object before deciding how to continue.

{{ code_snippet("adaptors/continuations_unwrap.cpp", "error") }}

However, continuations involve accessing the future object from the previous task. This means continuation chains where
the previous task is derived from a number of container adaptors can easily become verbose and error-prone. For
instance, consider a very simple continuation to a task that depends on 3 other futures objects.

This is the task for which we need a continuation.

{{ code_snippet("adaptors/continuations_unwrap.cpp", "verbose_create") }}

And this is how verbose the continuation looks like without unwrapping:

{{ code_snippet("adaptors/continuations_unwrap.cpp", "verbose_continue") }}

Although this pattern could be slightly simplified with more recent C++ features, such as structured bindings, this
pattern is unmaintainable. To simplify this process, the function [then] accepts continuations that expect the unwrapped
result from the previous task.

For instance, consider the following continuation function:

{{ code_snippet("adaptors/continuations_unwrap.cpp", "unwrap_void") }}

The continuation function requires no parameters. This means it only needs the previous future to be ready to be
executor, but it does not require to access the previous future object so a parameter of the previous type would be of
no use here. This also removes the necessity of marking the unused future object with attributes such
as [`[[maybe_unused]]`](https://en.cppreference.com/w/cpp/language/attributes/maybe_unused).

### Exceptions

If the previous task fails and its exception would be lost in the unwrapping process, the exception is automatically
propagated to the following task.

{{ code_snippet("adaptors/continuations_unwrap.cpp", "unwrap_fail_void") }}

With future adaptors, the exception information is still propagated to the unwrapped continuation future with the
underlying future objects being unwrapped. Thus, continuations without unwrapping are only necessary when (i) the
unwrapped version would lose the relevant exception information, and (ii) we need a different behavior for the
continuation. This typically happens when the continuation task contains some logic allowing us to recover from the
error.

### Value unwrapping

The simplest form of unwrapping is sending the internal future value directly to the continuation function.

{{ code_snippet("adaptors/continuations_unwrap.cpp", "value_unwrap") }}

This allows the continuation function to worry only about the internal value type `int` instead of the complete future
type `cfuture<int>`. This also makes the algorithm easier to generalize for alternative future types.

If the previous future also contains a future, we can double unwrap the value to the next task:

{{ code_snippet("adaptors/continuations_unwrap.cpp", "double_unwrap") }}

### Tuples unwrapping

Tuple unwrapping becomes useful as a simplified way for futures to return multiple values to its continuations.

{{ code_snippet("adaptors/continuations_unwrap.cpp", "tuple_unwrap") }}

The tuple components are also double unwrapped if necessary:

{{ code_snippet("adaptors/continuations_unwrap.cpp", "double_tuple_unwrap") }}

In this case, without unwrapping, the continuation would require
a `cfuture<std::tuple<vfuture<int>, vfuture<int>, vfuture<int>>>` as its first parameter.

### Unwrapping conjunctions

Double tuple unwrapping is one of the most useful types of future unwrapping for continuation functions of conjunctions.
When we wait for a conjunction of futures, the return value is represented as a tuple of all future objects that got
ready. Double tuple unwrapping allows us to handle the results is a pattern that is more manageable:

{{ code_snippet("adaptors/continuations_unwrap.cpp", "when_all_unwrap") }}

This allows the continuation function to worry only about the internal value type `int` instead of the complete future
type `cfuture<int>`. This also makes the algorithm easier to generalize for alternative future types.

### Unwrapping disjunctions

Future disjunctions are represented with instances of [when_any_result]. Special unwrapping functions are defined for
these objects. This simplest for of unwrapping for disjunctions is the index of the ready future and the previous
sequence of future objects.

{{ code_snippet("adaptors/continuations_unwrap.cpp", "when_any_unwrap") }}

This can still be as verbose as wrapped tuples. However, we might still want to have access to each individual future as
we know only one of them is ready when the continuation starts. So the second option is exploding the tuple of futures
into the continuation parameters.

{{ code_snippet("adaptors/continuations_unwrap.cpp", "when_any_explode_unwrap") }}

This pattern allows us to do something about `f2` when `f1` is ready and vice-versa. It implies we want to continue as
soon as there are results available but the results from unfinished tasks should not be discarded.

Very often, only one of the objects is really necessary and the meaning of what they store is homogenous. For instance,
this is the case when we attempt to connect to a number of servers and want to continue with whatever server replies
first. In this case, unfinished futures can be discarded, and we only need the finished task to continue.

{{ code_snippet("adaptors/continuations_unwrap.cpp", "when_any_single_result") }}

If the previous futures are stoppable, the adaptor will request other tasks to stop. If the tasks are homogeneous, this
means we can also unwrap the underlying value of the finished task.

{{ code_snippet("adaptors/continuations_unwrap.cpp", "when_any_single_result_unwrap") }}

### Summary

The following table describes all unwrapping functions by their priority:

| Future output                                                 | Continuation input                             | Inputs |
|---------------------------------------------------------------|------------------------------------------------|--------|
| `future<T>`                                                   | `future<T>`                                    | 1      |
| `future<T>`                                                   | ``                                             | 0      |
| `future<T>`                                                   | `T`                                            | 1      |
| `future<tuple<future<T1>, future<T2>, ...>>`                  | `future<T1>`, `future<T2>` ...                 | N      |
| `future<tuple<future<T1>, future<T2>, ...>>`                  | `T1`, `T2` ...                                 | N      |
| `future<vector<future<T>>>`                                   | `vector<T>`                                    | 1      |
| `future<when_any_result<tuple<future<T1>, future<T2>, ...>>>` | `size_t`, `tuple<future<T1>, future<T2>, ...>` | 2      |
| `future<when_any_result<tuple<future<T1>, future<T2>, ...>>>` | `size_t`, `future<T1>`, `future<T2>`, ...      | N + 1  |
| `future<when_any_result<tuple<future<T>, future<T>, ...>>>`   | `future<T>`                                    | 1      |
| `future<when_any_result<vector<future<T>>>>`                  | `future<T>`                                    | 1      |
| `future<when_any_result<tuple<future<T>, future<T>, ...>>>`   | `T`                                            | 1      |
| `future<when_any_result<vector<future<T>>>>`                  | `T`                                            | 1      |

Note that types are very important here. Whenever the continuation has the same number of arguments for the same future
output, a template function or a lambda using `auto` would be ambiguous.

{{ code_snippet("adaptors/continuations_unwrap.cpp", "ambiguous") }}

In this case, the continuation function will attempt to use the unwrapping with the highest priority, which would
be `cfuture<int>`. However, this is not always possible if the unwrapping overloads are ambiguous enough.

The continuation with the highest priority is always the safer and usually more verbose continuation. This means a
template continuation will usually unwrap to `future<T>` over `T` continuation input variants. On the other hand, this
is also useful since the most verbose continuation patterns are the ones that could benefit the most from `auto`.

## Return type unwrapping

Future are allowed to expect other futures:

{{ code_snippet("adaptors/continuations_unwrap.cpp", "return_future") }}

In this example, we can choose to wait for the value of the first future or the value of the future it encapsulates.

Unlike the function [std::experimental::future::then] in [C++ Extensions for Concurrency], this library does not
automatically unwrap a continuation return type from `future<future<int>>` to `future<int>`. There are two reasons for
that: not unwrapping the return type (i) facilitates generic algorithms that operate on futures, and (ii) avoids
potentially blocking the executor with two tasks to execute the unwrapping.

However, other algorithms based on the function [then] can still perform return type unwrapping.

## Continuation stop

When a non-shared future has a continuation attached, its value is moved into the continuation. With stoppable futures,
this means the [stop_source] is also moved into the continuation. If the future has already been moved, and we want to
request its corresponding task to stop, we can do that through its [stop_source].

{{ code_snippet("adaptors/continuations_unwrap.cpp", "stop_source") }}

--8<-- "docs/references.md"