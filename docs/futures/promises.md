# Promises and Packaged Tasks

## Promises

In some cases it might be necessary to have finer control over how the shared state is set. In these cases it might be
useful to have direct control over the promises and shared tasks.

A [promise] allows the user to directly control how the shared state should be set. By directly controlling the future
and the promise, the user has complete control over how the shared state is set. For instance, the promise value can be
set directly inline, returning a future whose value is immediately ready.

{{ code_snippet("tests/unit/snippets.cpp", "promise_inline") }}

In fact, this is the pattern behind functions such as [make_ready_future], which allows us to generate constant values
as futures, so that they can interoperate with other value objects.

In practice, the promise will usually be moved into a parallel execution context where its value will be set. This
becomes useful when the functionalities for launching tasks such as [async] and [schedule] do not offer enough control
over the process to set the promise value. With a [promise], it is possible directly control or even bypass executors.
For instance, a thread might be used to set the value of the shared state.

{{ code_snippet("tests/unit/snippets.cpp", "promise_thread") }}

This would be not directly possible with [async] because [std::thread] is not executor. This pattern can be replicated
with other types, such as [`boost::fiber`](https://www.boost.org/doc/libs/1_78_0/libs/fiber/doc/html/index.html).

This is equivalent to a more permanent solution which would be to define a custom executor that always launches tasks in
new threads.

{{ code_snippet("tests/unit/snippets.cpp", "promise_thread-executor") }}

This is how promises are related to the shared state:

<div class="mermaid">
graph LR
M[[User code 1]] --> |store|F[Future Value]
N[[User code 2]] --> |set|P[Value Promise]
subgraph Futures and Promises
F --> |read|S[(Shared State)]
P --> |write|S
end
M --> |store|P
M -.-> N[[User code 2]]
</div>

With a [promise] another library could be used to make a web request or run a process whose results will only be
available in the future.

By calling executor functions directly, we can also achieve a pattern that is very similar to [async]:

{{ code_snippet("tests/unit/snippets.cpp", "promise_thread_pool") }}

It's useful to note that promises allows us to define the functionalities of the future type via [future_options]. This
defines the type returned by
[`get_future`](/futures/reference/Classes/classfutures_1_1promise/#function-get_future).

{{ code_snippet("tests/unit/snippets.cpp", "promise_custom_options") }}

In this example, we explicitly define the promise should return a future with empty options. That is, the future has no
associated executor and does not support continuations. This is useful for immediately available futures because any
continuation to this future would not be required to poll for its results with [basic_future::wait].

## Packaged Tasks

The most common way to use promises is to wrap them in tasks that set their value. This pattern is simplified through
a [packaged_task], which stores a reference to the shared state and the task used to set its value.

<div class="mermaid">
graph LR
M[[User code 1]] --> |store|F[Future Value]
N[[User code 2]] --> |invoke|P[Packaged Task]
subgraph Futures and Promises
F --> |read|S[(Shared State)]
P --> |write|S
end
M --> |store|P
M -.-> N[[User code 2]]
</div>

The packaged task is a callable object that sets the shared state when invoked.

{{ code_snippet("tests/unit/snippets.cpp", "packaged_task_inline") }}

In this example, we immediately set the future value inline by invoking the packaged task. Instead of returning the
value of the task [packaged_task] stores, any value returned by the internal task is set as the future shared state.

When we pass a [packaged_task] as a parameter it acts like any other callable, which makes it more convenient than
promises for APIs that require callables such as when we create a [std::thread]:

{{ code_snippet("tests/unit/snippets.cpp", "packaged_task_thread") }}

By providing a [packaged_task] directly to an executor, we have a pattern that is very similar to [futures::async]:

{{ code_snippet("tests/unit/snippets.cpp", "packaged_task_executor") }}

Like promises, a [packaged_task] allows us to define the functionalities of the future type via [future_options]. This
defines the type to be returned later by
[`get_future`](/futures/reference/Classes/classfutures_1_1packaged__task_3_01R_07Args_8_8_8_08_00_01Options_01_4/#function-get_future)
.

--8<-- "docs/references.md"