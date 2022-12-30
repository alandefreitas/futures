# Launching tasks

## Future tasks

The easiest way to create futures is by launching tasks:

{{ code_snippet("test/unit/snippets.cpp", "no_param") }}

This is what happens under the hood:

<div class="mermaid">
graph LR
M[[Main Thread]] ==> |store|F[Future Value]
E[[Executor]] --> |run|T[Task]
M -.-> |launch|T
subgraph Futures and Promises
F --> |read|S[(Operation State)]
T[Promised Task] --> |write|S
end
</div>

1. An executor handles a task. Any data is stored in a private operation state.
2. While the executor handles a task, the main thread holds a future value.
3. When the task is completed, it fulfills its promise by setting the operation state with its result.
4. The future is considered ready and the main thread can obtain its value.

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

!!! hint "The Operation State"

    The operation state is a private implementation detail with 
    which the user does not interact. 

    This encapsulation ensures all write operations will happen
    through the promised task and all read operations will happen
    through the future.

    It also enables optimizations based on assumptions about how
    specific future and promise types can access the operation 
    state.

!!! hint "Inline Operation States"

    If these future and promise objects are stable during the execution
    of the task, the operation state can be stored inline. This is 
    an optimization that avoids dynamic memory allocations.    

## Eager tasks

Like [std::async], [futures::async] is used to launch new tasks in parallel.

{{ code_snippet("test/unit/snippets.cpp", "no_param") }}

In this example, this function returns a continuable [cfuture] by default instead of a [std::future]. If the first task
parameter is a [stop_source], it returns a [jcfuture] we can stop from the main thread with `request_stop`.

{{ code_snippet("test/unit/snippets.cpp", "no_param_jcfuture") }}

If the task accepts parameters, we can provide them directly to [async].

{{ code_snippet("test/unit/snippets.cpp", "with_params") }}

Unlike [std::async], which uses launch policies, [futures::async] can use any concrete [executor] specifying details
about how the task should be executed.

{{ code_snippet("test/unit/snippets.cpp", "with_executor") }}

If the [executor] is not defined, the default executor is used for the tasks. This executor ensures we do not launch a
new thread for each task.

To block execution of the main thread until the tasks are complete, we can use the functions [basic_future::wait]
and [basic_future::get].

{{ code_snippet("test/unit/snippets.cpp", "waiting") }}

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

!!! info "Callbacks and Eager Futures in C++"

    In [N3747](http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2013/n3747.pdf)
    , [Christopher Kohlhoff](https://github.com/chriskohlhoff) compares the model of eager continuations, such as
    in [N3784](http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2013/n3784.pdf) to the model of **callback functions** used in
    the [Asio](https://github.com/chriskohlhoff/asio) library:
    
    === "Futures"
    
        ```cpp
        void schedule_write(std::size_t length) {
            // the antecedent future represents the future result of `async_write`
            some_future_lib::async_write(socket_, asio::buffer(data_, length))
                // The continuation function creates a new future
                .then(schedule_read_function);
        }
        ``` 
    
    === "Callbacks"
    
        ```cpp
        void schedule_write(std::size_t length) {
            // async_write returns void
            asio::async_write(socket_, asio::buffer(data_, length), 
                // the continuation is provided when `async_write` schedules the first task
                schedule_read_function);
        }
        ``` 
    
    Eager continuable futures may attach the continuation *before or after* the antecedent future starts. With
    callbacks functions the calling function provides the continuation *before* the antecedent task starts, which
    **avoids the synchronization cost** in a race between the result and its continuation.

## Exceptions

Future objects defined in this library handle exceptions the same way [std::future] does. If the task throws an
exception internally, the exception is rethrown when we attempt to retrieve the value from the future
with [basic_future::get].

{{ code_snippet("test/unit/snippets.cpp", "throw_exception") }}

When working without exceptions, we can avoid terminating the process by querying the state of the future before
attempting to get its value.

{{ code_snippet("test/unit/snippets.cpp", "query_exception") }}

## Allocations

The operation state is a private implementation detail with which the user does not interact. To ensure stability,
it is usually implemented as a shared pointer to the concrete Operation State, which is where the task will store its
result.

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
this requires dynamic memory allocations of this shared state. For smaller tasks, the cost of this allocation might
dominate the time spent by the parallel task.

For this reason, functions used for launching futures allow custom memory allocators for eager tasks. When no
allocator is provided for launching tasks, an optimized memory pool allocator is provided for the operation state.

<div class="mermaid">
graph LR
F[Future] --> |read|S1[Shared State Pointer]
T[Promise] --> |write|S2[Shared State Pointer]
subgraph Shared State
S1 --> O[Operation State 1]
S2 --> O[Operation State 1]
end
P[Memory Pool] -.-> |owns|O[Operation State 1]
P[Memory Pool] -.-> |owns|O2[Operation State 2]
P[Memory Pool] -.-> |owns|O3[Operation State 3]
P[Memory Pool] -.-> |owns|Odots[...]
P[Memory Pool] -.-> |owns|ON[Operation State N]
</div>

The memory pool allows fast dynamic memory allocation and provides better cache locality for accessing the tasks
and their results.

However, in some circumstances, the library implements a few optimizations to avoid allocations altogether. In the
following example, we have a deferred future where no allocations are required.

{{ code_snippet("test/unit/snippets.cpp", "no_alloc") }}

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

## Executors

The concept defined for an [executor] uses Asio executors as defined
by [P1393r0](http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2019/p1393r0.html). This model for executors has been
evolving for over a decade and is widely adopted in C++. If the C++ standard eventually adopts a common vocabulary for
executors, the [executor] concept can be easily adjusted to handle these new executors types.

!!! info "Executors in C++"

    The first clear problem identified with [std::async] is we **cannot define its executor** (or scheduler). That is, we cannot
    define _where_ these tasks are executed. By default, every task is executed in a new thread in C++11, which is
    unacceptable to most applications. Common executors for these tasks would be thread pools, strands, or GPUs.
    
    Many [models for standard executors](http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2020/p0443r14.html) have been
    proposed [over time](http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2020/p0443r14.html#appendix-executors-bibilography)
    . In most current models, executors (or schedulers) are **light handles** representing an execution context _where_ work can
    be executed. The context is usually called an execution context or execution resource. Executors might be constructed
    directly from execution contexts or adapted from other executors.

!!! info "Better future executors"

    - Support for custom executors
        - Its traits are currently a subset of existing Asio executors.
    - Customization points can make other executor types work with the library types
    - Provides an alternative to [std::async] **based on [executors](#executors)**
        - Still adaptable to other executor or scheduler concepts, including senders and receivers.
        - The default executor does NOT create a new thread for each task

--8<-- "docs/references.md"