# Disjunctions

The function [when_any] is defined for task disjunctions. [when_any] returns a [when_any_future] that is able to
aggregate different futures types and become ready when any of the internal futures is ready.

Say we want to execute the following sequence of asynchronous tasks where the first future between `B` and `C` to get
ready should be used as input for the continuation `D`.

<div class="mermaid">
graph LR
subgraph Async
A --> B
A --> C
B --> |set if first|D
C --> |set if first|D
end
Main --> A
D --> End
Main --> End
</div>

Achieving that with the function [when_all] is as simple as:

{{ code_snippet("tests/unit/snippets.cpp", "when_any_small_graph") }}

The [when_any_future] object acts as a proxy that checks the state of each internal future. If none of the internal
futures is ready yet, [is_ready] returns `false`.

{{ code_snippet("tests/unit/snippets.cpp", "disjunction") }}

An instance of [when_any_future] returns a [when_any_result] which keeps the sequence of futures and the index of the
first future to get ready.

{{ code_snippet("tests/unit/snippets.cpp", "disjunction_result") }}

The underlying sequence might be a tuple or a range.

## Operator

The operator `||` is defined as a convenience to create future disjunctions in large task graphs.

{{ code_snippet("tests/unit/snippets.cpp", "operator_when_any") }}

With [when_any_result] unwrapping, this becomes a powerful tool to manage continuations:

{{ code_snippet("tests/unit/snippets.cpp", "disjunction_result_unwrap") }}

Note that the operator `&&` uses expression templates to create a single disjunction of futures. Thus, `f1 || f2 || f3`
is equivalent to `when_any(f1, f2, f3)` rather than `when_any(when_any(f1, f2), f3)`.

The operator `||` can also be used with lambdas as an easy way to launch new tasks already into disjunctions:

{{ code_snippet("tests/unit/snippets.cpp", "when_any_operator_lambda") }}

This allows us to assemble task graphs where lambdas are treated as other first class types. The types accepted by these
operators are limited to those matching the future concept and callables that are valid as new tasks.

## Lazy continuations

Unlike [when_all_future], the behavior of [when_any_future] is strongly affected by the features of its internal
futures. Internal futures that support lazy continuations and external notifiers can only set a flag indicating
to [when_any_future] that one of its futures is ready.

For internal futures that do not support lazy continuations, [when_any_future] needs to pool its internal future to
check if any is ready, much like [wait_for_any].

## Disjunction unwrapping

Special unwrapping functions are defined for [when_any_future] continuations.

{{ code_snippet("tests/unit/snippets.cpp", "when_any_unwrap") }}

{{ code_snippet("tests/unit/snippets.cpp", "when_any_explode_unwrap") }}

{{ code_snippet("tests/unit/snippets.cpp", "when_any_single_result") }}

{{ code_snippet("tests/unit/snippets.cpp", "when_any_single_result_unwrap") }}

--8<-- "docs/references.md"