# Launching tasks

## Futures tasks

The easiest way to create futures is by launching tasks:

{{ code_snippet("future_types/launching.cpp", "no_param") }}

This is what happens under the hood:

<div class="mermaid">
graph LR
M[[Main Thread]] ==> |store|F[Future Value]
E[[Executor]] --> |run|T[Task]
M -.-> |launch|T
subgraph Futures and Promises
F --> |read|S[(Shared State)]
T[Promised Task] --> |write|S
end
</div>

1. While an executor handles a task, the main thread holds a future value.
2. When the task is completed, it fulfills its promise by setting the shared state with its result.
3. The future is considered ready and the main thread can obtain its value.

This is how these three steps might happen:

<div class="mermaid">
sequenceDiagram
    rect rgb(191, 200, 255)
    Main->>Task: Launch
    Task->>State: Create
    activate Task
    State->>Future: Create
    Task->>Task: Executor runs task
    Future->>Main: Store
    end
    Main->>Main: Do work before waiting
    rect rgb(191, 223, 200)
    Main->>Future: Wait
    Future->>State: Wait
    Task->>State: Write
    deactivate Task
    Future->>State: Read
    end
    rect rgb(150, 223, 255)
    Future->>Main: Return
    end
</div>

!!! hint "The shared state"

    The shared state is a private implementation detail with which the user does not interact. 

    This encapsulation ensures all write operations will happen through the promised task and all read operations 
    will happen through the future.

    It also enables optimizations based on assumptions about how specific future and promise types can access the 
    shared state. In some circumstances, the shared state might not even need to be [allocated](#allocations). 

## Eager tasks

Like [std::async], [futures::async] is used to launch new tasks in parallel.

{{ code_snippet("future_types/launching.cpp", "no_param") }}

In this example, this function returns a continuable [cfuture] by default instead of a [std::future]. If the first task
parameter is a [stop_source], it returns a [jcfuture] we can stop from the main thread with `request_stop`.

{{ code_snippet("future_types/launching.cpp", "no_param_jcfuture") }}

If the task accepts parameters, we can provide them directly to [async].

{{ code_snippet("future_types/launching.cpp", "with_params") }}

Unlike [std::async], which uses launch policies, [futures::async] can use any concrete [executor] specifying details
about how the task should be executed.

{{ code_snippet("future_types/launching.cpp", "with_executor") }}

If the [executor] is not defined, the default executor is used for the tasks. This executor ensures we do not launch a
new thread for each task.

To block execution of the main thread until the tasks are complete, we can use the functions [basic_future::wait]
and [basic_future::get].

{{ code_snippet("future_types/launching.cpp", "waiting") }}

The function [basic_future::wait] will only block execution until the task is ready, while [basic_future::get] can be
used to wait for and retrieve the final value.

Note that eager tasks are allowed to start as soon as they are launched:

<div class="mermaid">
sequenceDiagram
    Main->>+Task: Launch
    Task->>Task: Do work
    Main->>Main: Do work
    Main->>Task: Wait
    Task->>-Main: Return
</div>

## Deferred tasks

The function [schedule] can be used to create lazy tasks.

{{ code_snippet("future_types/schedule.cpp", "schedule") }}

The main difference between [async] and [schedule] is the latter only posts the task to the executor when we
call [basic_future::wait] or [basic_future::get] on the corresponding future:

<div class="mermaid">
sequenceDiagram
    Main->>Task: Schedule
    Main->>+Task: Wait
    Task->>Task: Do work
    Task->>-Main: Return
</div>

Most of the time, the choice between eager and lazy futures is determined by the application. When both eager and lazy
futures are applicable, a few criteria might be considered.

- Eager futures ???: have the obvious benefit of allowing us to already start with the tasks we know about before
  assembling the complete execution graph. This is especially useful when not all tasks are available at the same time.
- Deferred futures ???: On the other hand, lazy futures permit a few optimizations for functions operating on the shared
  state, since we can assume there is nothing else we need to synchronize when a task is launched. This means:
    1. We can make extra assumptions about the condition of the shared state before waiting, and
    2. We don't have to handle a potential race with the task and its own continuations
- Eager futures ???: The library implements the synchronization of eager futures using atomic operations to reduce this
  synchronization cost.
- Both futures ???: For applications with reasonably long tasks, the difference between the two is likely to be
  negligible.

!!! hint "Deferred Future Continuations"

    The library assumes continuations to deferred futures are always attached *before*
    the future starts executing. 

    This strategy uses deferred futures as an oportunity for an extra optimization where 
    we don't have to synchronize the access to continuations by the thread attaching the
    continuation and the main task. 

    If the deferred future is already executing in one thread, this means it is *not thread safe*
    to attempt to attach a continuation in a second thread.      

## Ready tasks

When assembling task graphs, it's often useful to include constant values for which we already know the result but
behave like a future type. This can be achieved through [make_ready_future]:

{{ code_snippet("future_types/launching.cpp", "ready_future") }}

The function returns a [vfuture], which represents a [basic_future] with no associated shared state extensions.

## Executors

The concept defined for an [executor] uses Asio executors as defined
by [P1393r0](http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2019/p1393r0.html). This model for executors has been
evolving for over a decade and is widely adopted in C++. If the C++ standard eventually adopts a common vocabulary for
executors, the [executor] concept can be easily adjusted to handle these new executors types.

## Exceptions

Future objects defined in this library handle exceptions the same way [std::future] does. If the task throws an
exception internally, the exception is rethrown when we attempt to retrieve the value from the future
with [basic_future::get].

{{ code_snippet("future_types/launching.cpp", "throw_exception") }}

When working without exceptions, we can avoid terminating the process by querying the state of the future before
attempting to get its value.

{{ code_snippet("future_types/launching.cpp", "query_exception") }}

## Allocations

The shared state is a private implementation detail with which the user does not interact. It is usually implemented as
a shared pointer to the concrete Operation State, which is where the task will store its result.

<div class="mermaid">
graph LR
F[Future] --> |read|S1[Shared State Pointer]
T[Promise] --> |write|S2[Shared State Pointer]
subgraph Shared State
S1 --> O[Operation State]
S2 --> O[Operation State]
end
</div>

In the general case, the operation state needs a stable address so that futures and promises can access it. In turn,
this requires dynamic allocation of this shared state. For smaller tasks, the cost of this allocation might dominate the
time spent by the parallel task.

For this reason, functions used for launching futures allow custom memory allocators for the shared tasks. Because a
chain of futures implicitly work as task queue, simple and efficient linear allocators can reduce this allocation cost
reasonably. When no allocator is provided to launching functions, such an optimized allocator is provided for the
allocating the operation state.

However, in some circumstances, the library implements a few optimizations to avoid allocations altogether. In the
following example, we have a deferred future where no allocations are required.

{{ code_snippet("future_types/schedule.cpp", "no_alloc") }}

In this example, the operation state might be stored inline with the future:

<div class="mermaid">
graph RL
F[Future<br>+<br>Operation State] --> |read|F
T[Promise] --> |write|F
</div>

With this optimization, the operation state is stored in the future, so the future can read the results from its own
operation state. The promise can use the address of the operation state in the future to write its result. In this case,
the promise is **connected** with the future and can store results directly in its operation state.

This optimization is only applicable if the address of the future will not change, i.e.: will not be moved or destroyed,
during task execution, i.e.: (i) after the task to set the operation state is launched and (ii) before the operation
state is set. This is easiest to achieve in deferred futures, because the thread waiting for its result is blocked when
we call [basic_future::wait] and the future can only be moved or destroyed after the underlying operation state is set.

Note that this optimization is only possible if we can ensure the future cannot be moved or destroyed during task
execution:

- Functions such as [basic_future::wait_for] will disable this optimization for deferred futures because the underlying
  object might be moved after [basic_future::wait_for] times out and during task execution.
- This optimization can be enabled for eager futures that we know should not be moved or destroyed during task
  execution. This is common in applications represented as classes that store their future instances. If we mistakenly
  attempt to move or destroy the future during execution, it blocks the calling thread until the operation state is set.
- Similarly, this optimization can be enabled for shared futures if the future object from where the task is launched is
  not moved or destroyed during execution.

--8<-- "docs/references.md"