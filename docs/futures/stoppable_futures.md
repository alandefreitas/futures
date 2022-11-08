# Stoppable Futures

When calling [async] with a callable that can be called with a [stop_token] as its first argument, it
returns [jcfuture]:

{{ code_snippet("tests/unit/snippets.cpp", "stoppable") }}

The [stop_token] defined by the library is similar to the standard defined for [std::stop_token]. In this example, the
task won't be ready until we ask it to stop through the state [stop_source].

{{ code_snippet("tests/unit/snippets.cpp", "not_ready") }}

We can use the function [basic_future::request_stop] to ask the task to stop through its future object.

{{ code_snippet("tests/unit/snippets.cpp", "request_stop") }}

The shared state of a [jcfuture] contains a [stop_source] which can be used to request the task to stop from another
execution context:

<div class="mermaid">
graph LR
M[[Main Thread]] ==> |store|F[Future Value]
M -.-> |request stop!|F
E[[Executor]] --> |run|T[Task]
M -.-> |launch|T
subgraph Futures and Promises
F --> |read|S[(Shared State <br> + <br> Stop State)]
F -.-> |request stop!|S
T[Task] --> |write|S
T -.-> |should stop?|S
end
</div>

This feature is defined as an additional write permission for the future object to the stop state.

--8<-- "docs/references.md"