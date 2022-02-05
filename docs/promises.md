# Promises and Packaged Tasks

In some cases it might be necessary to have finer control over how the shared state is set. In
these cases it might be useful to have direct control over the promises and shared tasks.

## Promises

A [promise] allows the user to directly control how the shared state should be set. With a [promise],
it is possible directly control or even bypass executors.

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

By directly controlling the future and the promise, the user has complete control
over how the shared state is set. For instance, the promise value can be set 
directly inline, returning a future whose value is almost immediately ready. 

In practice, the promise will usually be moved into a parallel execution context 
where its value will be set. This becomes useful when the functionalities for
launching tasks such as [async] and [schedule] do not offer enough control
over the process to set the promise value:  

- For instance, a thread might be used to set the value of the shared state.
This is not directly possible with [async] because `std::thread` is not executor.
- Alternatively, another library could be used to make a web request or run a process
whose results will only be available in the future.

```cpp
--8<-- "examples/future_types/promises.cpp"
```

It's useful to note that promises allows us to define the functionalities of 
the future type via [future_options]. This defines the type returned by 
[`get_future`](/futures/reference/Classes/classfutures_1_1promise/#function-get_future).

## Packaged Tasks

The most common way to use promises is to wrap them in tasks that set their value.
This pattern is simplified through a [packaged_task], which stores a reference
to the shared state and the task used to set its value.

<div class="mermaid">
graph LR
M[[User code 1]] --> |store|F[Future Value]
N[[User code 2]] --> |invoke|P[Value Promise]
subgraph Futures and Promises
F --> |read|S[(Shared State)]
P --> |write|S
end
M --> |store|P
M -.-> N[[User code 2]]
</div>

The packaged task is a callable object. When we pass a [packaged_task] to an executor
it looks like a regular function. However, when the function is invoked, it sets
the shared value with the result instead of returning it.

```cpp
--8<-- "examples/future_types/packaged_task.cpp"
```

Like promises, a [packaged_task] allows us to define the functionalities of
the future type via [future_options]. This defines the type returned by
[`get_future`](/futures/reference/Classes/classfutures_1_1packaged__task_3_01R_07Args_8_8_8_08_00_01Options_01_4/#function-get_future).


--8<-- "docs/references.md"