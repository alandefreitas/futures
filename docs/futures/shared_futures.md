# Shared Futures

In a shared future, multiple tasks are allowed to wait for and depend on the shared state with another promise.

<div class="mermaid">
graph TB
F1[Shared Future] --> |read|S[(Shared State)]
F2[Shared Future] --> |read|S[(Shared State)]
F3[...] --> |read|S[(Shared State)]
F4[Shared Future] --> |read|S[(Shared State)]
P[Promise] --> |write|S
</div>

A shared future can be created from a regular future with the function [basic_future::share]:

{{ code_snippet("tests/unit/snippets.cpp", "create_shared") }}

When creating a shared future, the previous future value is consumed, and it becomes invalid.

{{ code_snippet("tests/unit/snippets.cpp", "invalidate_unique") }}

For this reason, it's common to create shared futures in a single step.

{{ code_snippet("tests/unit/snippets.cpp", "single_step") }}

The main difference between a regular future and a shared future is that many valid futures might refer to the same
shared state:

{{ code_snippet("tests/unit/snippets.cpp", "share_state") }}

While a regular future object is invalidated when its value is read, a value shared by many futures can be read multiple
times.

{{ code_snippet("tests/unit/snippets.cpp", "get_state") }}

However, this comes at a cost. The main consideration when using a shared future is that the value returned by the
[`get`](/futures/reference/Classes/classfutures_1_1basic__future/#function-get) function is not moved outside the
future.

{{ code_snippet("tests/unit/snippets.cpp", "get_state") }}

This means the reference returned by [basic_future::get] needs to be handled carefully. If its value is attributed to
another object of the same time, this involves copying the value instead of moving it. For this reason, we always use
unique futures as a default.

{{ code_snippet("tests/unit/snippets.cpp", "future_vector") }}

{{ code_snippet("tests/unit/snippets.cpp", "shared_future_vector") }}

Having said that, there are applications where sharing the results from the future are simply necessary. If multiple
tasks depend on the result of the previous task, there might be no alternative to shared futures. In some applications,
futures might contain only trivial values that are cheap to copy. In other applications, sharing a reference or a 
shared pointer by value without making copies might also be acceptable.

Like [std::future] and [std::shared_future], the classes [jfuture], [cfuture], [jcfuture] also have their shared
counterparts [shared_jfuture], [shared_cfuture], [shared_jcfuture]. In fact, any future defined by [basic_future] has
its shared counterpart where its [future_options] define the state as shared.

--8<-- "docs/references.md"