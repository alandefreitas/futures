# Conjunctions

## Conjunctions

Like in [C++ Extensions for Concurrency], the [when_all] function is defined for task conjunctions. Say we want to
execute the following sequence of asynchronous tasks:

<div class="mermaid">
graph LR
subgraph Async
A --> B
A --> C
B --> D
C --> D
end
Main --> A
D --> End
Main --> End
</div>

Achieving that with the function [when_all] is as simple as:

{{ code_snippet("adaptors/conjunctions.cpp", "small_graph") }}

The function [when_all] returns a [when_all_future] that is a future adaptor able to aggregate different futures types
and become ready when all internal futures are ready.

{{ code_snippet("adaptors/conjunctions.cpp", "conjunction") }}

When retrieving results, a tuple with the original future objects is returned.

{{ code_snippet("adaptors/conjunctions.cpp", "conjunction_return") }}

When a range is provided to [when_all], another range is returned. The [when_all_future] object acts as a proxy object
that checks the state of each internal future. If any of the internal futures isn't ready yet, [is_ready]
returns `false`.

{{ code_snippet("adaptors/conjunctions.cpp", "conjunction_range") }}

## Operators

The operator `&&` is defined as a convenience to create future conjunctions in large task graphs.

{{ code_snippet("adaptors/conjunctions.cpp", "operator") }}

With tuple unwrapping, this becomes a powerful tool to manage continuations:

{{ code_snippet("adaptors/conjunctions.cpp", "continuation_unwrap") }}

Note that the operator `&&` uses expression templates to create a single conjunction of futures. Thus, `f1 && f2 && f3`
is equivalent to `when_all(f1, f2, f3)` rather than `when_all(when_all(f1, f2), f3)`.

The operator `&&` can also be used with lambdas as an easy way to launch new tasks already into conjunctions:

{{ code_snippet("adaptors/conjunctions.cpp", "operator_lambda") }}

This makes lambdas a first class citizen when composing task graphs. The types accepted by these operators only
participate in overload resolution if they match the future concept or are callables that are valid new asynchronous
tasks. This avoids conflicts with operator overloads defined for other types.

## Conjunction unwrapping

The tuple unwrapping functions and especially double unwrapping are especially useful for [when_all_future]
continuations.

{{ code_snippet("adaptors/continuations_unwrap.cpp", "when_all_unwrap") }}

--8<-- "docs/references.md"